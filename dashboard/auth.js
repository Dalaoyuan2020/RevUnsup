// RevUnsup Dashboard - Front-door auth (前端密码门)
// 注意：这只是挡路人的门，不是真正的安全。GitHub Pages 是纯静态托管，
// 任何前端密码都能被查源码看到。详见 README v0.2 说明。

(function() {
    const PWD_HASH = "158a323a7ba44870f23d96f1516dd70aa48e9a72db4ebb026b0a89e212a208ab";
    const SESSION_KEY = "revunsup_auth_v1";

    async function sha256(str) {
        const buf = new TextEncoder().encode(str);
        const hashBuf = await crypto.subtle.digest("SHA-256", buf);
        return Array.from(new Uint8Array(hashBuf))
            .map(b => b.toString(16).padStart(2, "0"))
            .join("");
    }

    async function checkAuth() {
        if (sessionStorage.getItem(SESSION_KEY) === "ok") return true;
        for (let i = 0; i < 3; i++) {
            const pwd = prompt("RevUnsup Dashboard 访问密码：");
            if (pwd === null) {
                document.body.innerHTML = '<div style="color:#888;font-family:sans-serif;text-align:center;margin-top:40vh;">已取消访问</div>';
                return false;
            }
            const h = await sha256(pwd);
            if (h === PWD_HASH) {
                sessionStorage.setItem(SESSION_KEY, "ok");
                return true;
            }
            alert("密码错误，剩余尝试次数：" + (2 - i));
        }
        document.body.innerHTML = '<div style="color:#ef4444;font-family:sans-serif;text-align:center;margin-top:40vh;">访问被拒绝</div>';
        return false;
    }

    // 在 DOM 准备前先把页面隐藏，避免内容闪现
    document.documentElement.style.visibility = "hidden";

    function reveal() {
        document.documentElement.style.visibility = "visible";
    }

    checkAuth().then(ok => {
        if (ok) reveal();
    });
})();
