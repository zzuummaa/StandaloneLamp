var updateStatusTimeout = 2000;

function updateStatus() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', window.location.origin + "/api/status", true);
    xhr.timeout = updateStatusTimeout;

    xhr.onreadystatechange = function() {
        if (xhr.readyState != 4) return;

        if (xhr.status != 200) {
            setMsg("error", "HTTP Server respond, but with error.");
        } else {
            setMsg("done", "HTTP Server is up and running.");
        }
    };

    xhr.ontimeout = function() {
        setMsg("error", "HTTP Server not respond.");
    };

    xhr.send();
    setTimeout(updateStatus, updateStatusTimeout);
}

function setMsg(cls, text) {
    sbox = document.getElementById('server_status');
    sbox.className = "alert alert-" + cls;
    sbox.innerHTML = text;
}

function requestWifiSettings(onSuccess) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', window.location.origin + "/api/wifi", true);

    xhr.onreadystatechange = function() {
        if (xhr.readyState != 4) return;
        if (xhr.status == 200) {
            var json = JSON.parse(xhr.responseText);
            onSuccess(json)
        }
    };

    xhr.send();
}