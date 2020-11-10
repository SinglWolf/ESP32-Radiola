var content = "Content-type",
    ctype = "application/x-www-form-urlencoded",
    cjson = "application/json";
var auto, intervalid, intervalrssi, timeid, websocket, urlmonitor, monitor, e, moniPlaying = false,
    editPlaying = false,
    editIndex = 0,
    curtab = "PLAYER",
    stchanged = false,
    maxStation = 255;
const mediacentre = "Радиола";
const working = "Работаю... Пожалуйста, подождите.";
const _ERR = "ОШИБКА ", _default = "ПО УМОЛЧАНИЮ", _loadNVS = "СЧИТАНО ИЗ NVS", _saveNVS = "ЗАПИСАНО В NVS", _noNVS = "Записей в NVS ещё нет!!!";
const backupConsole = console;

function openwebsocket() {
    //	autoplay(); //to force the server socket to accept and open the web server client.
    websocket = new WebSocket("ws://" + window.location.host + "/");
    console.log("url:" + "ws://" + window.location.host + "/");


    websocket.onmessage = function (event) {
        try {
            var arr = JSON.parse(event.data);
            console.log("onmessage:" + event.data);
            if (arr["meta"] == "") {
                document.getElementById('meta').innerHTML = mediacentre;
            }
            if (arr["meta"]) {
                document.getElementById('meta').innerHTML = arr["meta"].replace(/\\/g, "");
            }
            changeTitle(document.getElementById('meta').innerHTML);
            if (arr["wsvol"]) onRangeVolChange(arr['wsvol'], false);
            if (arr["wsicy"]) icyResp(arr["wsicy"]);
            if (arr["wssound"]) soundResp(arr["wssound"]);
            if (arr["monitor"]) playMonitor(arr["monitor"]);
            if (arr["wsstation"]) wsplayStation(arr["wsstation"]);
            if (arr["ircode"]) { document.getElementById('new_key').innerHTML = arr["ircode"]; }
            if (arr["upgrade"]) { document.getElementById('updatefb').innerHTML = arr["upgrade"]; }
            if (arr["iurl"]) {
                document.getElementById('instant_url').value = arr["iurl"];
                buildURL();
            }
            if (arr["ipath"]) {
                document.getElementById('instant_path').value = arr["ipath"];
                buildURL();
            }
            if (arr["iport"]) {
                document.getElementById('instant_port').value = arr["iport"];
                buildURL();
            }
            if (arr["wsrssi"]) {
                document.getElementById('rssi').innerHTML = arr["wsrssi"] + ' дБм';
                setTimeout(wsaskrssi, 5000);
            }
            if (arr["wscurtemp"]) {
                document.getElementById('curtemp').innerHTML = arr["wscurtemp"] + ' ℃';
            }
            if (arr["wsrpmfan"]) {
                document.getElementById('rpmfan').innerHTML = arr["wsrpmfan"] + ' Об/м';
            }
        } catch (e) { console.log("error" + e); }
    }

    websocket.onopen = function (event) {
        console.log("Open, url:" + "ws://" + window.location.host + "/");
        if (window.timerID) { /* a setInterval has been fired */
            window.clearInterval(window.timerID);
            window.timerID = 0;
        }
        setTimeout(wsaskrssi, 5000); // start the rssi display
        websocket.send("opencheck");
    }
    websocket.onclose = function (event) {
        console.log("onclose code: " + event.code);
        console.log("onclose reason: " + event.reason);
        if (!window.timerID) { /* avoid firing a new setInterval, after one has been done */
            window.timerID = setInterval(function () { checkwebsocket() }, 1500);
        }
    }
    websocket.onerror = function (event) {
        // handle error event
        console.log("onerror reason: " + event.reason);
        websocket.close();
    }
}

// Change the title of the page
function changeTitle($arr) {
    window.parent.document.title = $arr; // on change l'attribut <title>
}

// ask for the rssi and restart the timer
function wsaskrssi() {
    try {
        websocket.send("wsrssi &");
    } catch (e) { console.log("error" + e); }
}

function wsplayStation($arr) {
    var i;
    var select = document.getElementById('stationsSelect');
    var opt = select.options;
    for (i = 0; i < maxStation; i++) {
        var id = opt[i].value.split(":");
        id = id[0];
        if (id == $arr) break;
    }
    if (i == maxStation) i = 0;
    select.selectedIndex = i;
}

function playMonitor($arr) {
    urlmonitor = "";
    urlmonitor = $arr;
    if (moniPlaying) {
        monitor = document.getElementById("audio");
        if (urlmonitor.endsWith("/"))
            var src = urlmonitor + ";";
        else src = urlmonitor;
        monitor.src = src;
        monitor.play();
    }
}

function mplay() {
    monitor = document.getElementById("audio");
    if (urlmonitor.endsWith("/"))
        var src = urlmonitor + ";";
    else src = urlmonitor;
    monitor.src = src;
    monitor.volume = document.getElementById("volm_range").value / 100;
    while (monitor.networkState == 2) { };
    monitor.play();
    moniPlaying = true;
    monitor.muted = false;
}

function monerror() {
    monitor = document.getElementById("audio");
    console.log("monitor error1 " + monitor.error.code);
}

function mstop() {
    monitor = document.getElementById("audio");
    monitor.muted = true;
    monitor.src = 'https://serverdoma.ru/esp32/silence-1sec.mp3';
    moniPlaying = false;
}

function mpause() {
    monitor = document.getElementById("audio");
    monitor.pause();
}

function mvol($name, $step) {
    var val = parseInt(document.getElementById($name + '_range').value, 10) + $step;
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    document.getElementById($name + '_range').value = val;
    document.getElementById($name + '_span').innerHTML = val + "%";
    monitor = document.getElementById("audio");
    monitor.volume = val / 100;
}

function checkwebsocket() {
    if (typeof websocket == 'undefined') openwebsocket();
    else {
        if (websocket.readyState == websocket.CLOSED) openwebsocket();
        else if (websocket.readyState == websocket.OPEN) websocket.send("opencheck");
    }
}

//check for a valid ip	
function chkip($this) {
    if (/^([0-9]+\.){3}[0-9]+$/.test($this.value)) $this.style.color = "green";
    else $this.style.color = "red";
}

function clickdhcp() {
    if (document.getElementById("dhcp").checked) {
        document.getElementById("ip").setAttribute("disabled", "");
        document.getElementById("mask").setAttribute("disabled", "");
        document.getElementById("gw").setAttribute("disabled", "");
    } else {
        document.getElementById("ip").removeAttribute("disabled");
        document.getElementById("mask").removeAttribute("disabled");
        document.getElementById("gw").removeAttribute("disabled");
    }
}

function consoleON() {
    if (document.getElementById("consON").checked) {
        console = backupConsole;
    } else {
        console = {
            log: () => { },
            error: () => { },
            warn: () => { }
        };
    }
}

function valid() {
    wifi(1);
    alert("Перезагрузка Радиолы. Пожалуйста, измените адрес страницы вашего браузера на новый.");
}

function abortIrKey() {
    document.getElementById('editIrKeyDiv').style.display = "none";
    ircode(0);
}

function eraseIrKey() {
    document.getElementById('current_key').innerHTML = "";
    document.getElementById('K_' + document.getElementById('key_num').value).innerHTML = "";
}
function saveIrKey(num) {
    var val = document.getElementById('new_key').innerHTML;
    if (val == "") {
        alert("Сначала нажмите кнопку пульта!");
    } else {
        var checkIR = 0;
        var checkir = false;

        for (var i = 0; i < 17; i++) {
            if (document.getElementById('K_' + i).innerHTML == val) checkIR++;
        }

        if (checkIR >= 1) checkir = true;
        console.log(checkir);
        if (document.getElementById('current_key').innerHTML == val) {
            checkir = true;
        }
        if (window.checkir == true) {
            alert("Такой код уже назначен!");
            document.getElementById('new_key').innerHTML = "";
        }
        else {
            document.getElementById('K_' + num).innerHTML = val;
            // document.getElementById('K_' + num).style = "color:red;";
            document.getElementById('K_' + num).style.color = "red";
            abortIrKey();
        }
    }
}

function clearAllKey() {
    if (confirm("Стереть все текущие коды пульта?")) {
        for (var i = 0; i < 17; i++) {
            document.getElementById('K_' + i).innerHTML = "";
        }
    }
}

function training(key, num) {
    document.getElementById('editIrKeyDiv').style.display = "block";
    document.getElementById('key_name').innerHTML = key;
    document.getElementById('current_key').innerHTML = document.getElementById('K_' + num).innerHTML;
    document.getElementById('key_num').value = num;
    document.getElementById('new_key').innerHTML = "";
    ircode(1);
}

function ircodes(ir_mode, save, load) {
    var setircodes = "", ir_mode_txt, err, style_color, xhr;
    xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var arr = JSON.parse(xhr.responseText);
            if (load == 1) {
                ir_mode = parseInt(arr["IR_MODE"].replace(/\\/g, ""), 10);
            }
            err = arr["ERROR"].replace(/\\/g, "");
            if (err != "0000") {
                ir_mode_txt = _ERR + err + "<br>";
                if (err == "1102")
                    ir_mode_txt = ir_mode_txt + _noNVS;
                style_color = "red";
            } else {
                if (ir_mode == 0) {
                    ir_mode_txt = _default;
                    style_color = "blue";
                } else {
                    if (save == 0) {
                        ir_mode_txt = _loadNVS;
                    } else {
                        ir_mode_txt = _saveNVS;
                    }
                    style_color = "green";
                }
            }
            document.getElementById("IrErr").style.color = style_color;
            document.getElementById("IrErr").innerHTML = ir_mode_txt;
            for (var i = 0; i < 17; i++) {
                document.getElementById('K_' + i).innerHTML = arr["K_" + i];
            }
        }
    }
    if ((save == 1) && (confirm("Записать коды пульта в NVS?"))) {
        for (var i = 0; i < 17; i++) {
            setircodes += "&K_" + i + "=" + document.getElementById('K_' + i).innerHTML;
        }
    } else save = 0;

    xhr.open("POST", "ircodes", false);
    xhr.setRequestHeader(content, ctype);
    xhr.send("ir_mode=" + ir_mode +
        "&save=" + save +
        setircodes +
        "&");
}


function scrollTo(to, duration) {
    if (duration < 0) return;
    var scrollTop = document.body.scrollTop + document.documentElement.scrollTop;
    var difference = to - scrollTop;
    var perTick = difference / duration * 10;

    setTimeout(function () {
        scrollTop = scrollTop + perTick;
        document.body.scrollTop = scrollTop;
        document.documentElement.scrollTop = scrollTop;
        if (scrollTop === to) return;
        scrollTo(to, duration - 10);
    }, 10);
}

// go to top
function wtop() {
    //	window.scrollTo(0, 0);
    scrollTo(0, 200);
}

// display current time
function dtime() {
    var d = new Date(),
        elt = document.getElementById('sminutes'),
        eltw = document.getElementById('wminutes');
    //	document.getElementById("time").innerHTML = d.toLocaleTimeString();	
    var udate = d.toTimeString().split(" ");
    document.getElementById("time").innerHTML = udate[0];
    if ((!isNaN(elt.innerHTML)) && (elt.innerHTML != "0"))
        --elt.innerHTML;
    if ((!isNaN(elt.innerHTML)) && (elt.innerHTML == 0)) elt.innerHTML = "0";
    if ((!isNaN(eltw.innerHTML)) && (eltw.innerHTML != "0"))
        --eltw.innerHTML;
    if ((!isNaN(eltw.innerHTML)) && (eltw.innerHTML == 0)) eltw.innerHTML = "0";
}

function labelSleep(label) {
    document.getElementById('sminutes').innerHTML = label;
}

function sleepup(e) {
    if (e.keyCode == 13) startSleep();
}

function startSleep() {
    var valm, h0, h1;
    var cur = new Date();
    var hop = document.getElementById("sleepdelay").value.split(":");
    h0 = parseInt(hop[0], 10);
    h1 = parseInt(hop[1], 10);
    if (!isNaN(h0)) {
        if (!isNaN(h1)) // time mode
        {
            var fut = new Date(cur.getFullYear(), cur.getMonth(), cur.getDate(), h0, h1, 0);
            if (fut.getTime() > cur.getTime())
                valm = Math.round((fut.getTime() - cur.getTime()) / 60000); //seconds
            else valm = 1440 - Math.round((cur.getTime() - fut.getTime()) / 60000); //seconds
            if (valm == 0)
                if (fut.getTime() > cur.getTime()) valm = 1; // 1 minute mini
                else valm = 1440;
        } else valm = h0; // minute mode
        websocket.send("startSleep=" + valm + "&");
        labelSleep("Запущено, спокойной ночи!");
        window.setTimeout(labelSleep, 2000, (valm * 60) - 2);
    } else {
        labelSleep("Ошибка, попробуйте еще раз");
        window.setTimeout(labelSleep, 2000, "0");
    }
}

function stopSleep() {
    var a = document.getElementById("sleepdelay").value;
    websocket.send("stopSleep");
    labelSleep("0");
    //    window.setTimeout(labelSleep, 2000, a);	
}

function labelWake(label) {
    document.getElementById('wminutes').innerHTML = label;
}

function wakeup(e) {
    if (e.keyCode == 13) startWake();
}

function startWake() {
    var valm, h0, h1;
    var cur = new Date();
    var hop = document.getElementById("wakedelay").value.split(":");
    h0 = parseInt(hop[0], 10);
    h1 = parseInt(hop[1], 10);
    if (!isNaN(h0)) {
        if (!isNaN(h1)) // time mode
        {
            var fut = new Date(cur.getFullYear(), cur.getMonth(), cur.getDate(), h0, h1, 0);
            if (fut.getTime() > cur.getTime())
                valm = Math.round((fut.getTime() - cur.getTime()) / 60000); //seconds
            else valm = 1440 - Math.round((cur.getTime() - fut.getTime()) / 60000); //seconds
            if (valm == 0)
                if (fut.getTime() > cur.getTime()) valm = 1; // 1 minute mini
                else valm = 1440;
        } else valm = h0; // minute mode
        websocket.send("startWake=" + valm + "&");
        labelWake("Запущено");
        window.setTimeout(labelWake, 2000, (valm * 60) - 2);
    } else {
        labelWake("Ошибка, попробуйте еще раз");
        window.setTimeout(labelWake, 2000, "0");
    }
}

function stopWake() {
    var a = document.getElementById("wakedelay").value;
    websocket.send("stopWake");
    labelWake("0");
}



function promptworking(label) {
    document.getElementById('meta').innerHTML = label;
    if (label == "") {
        document.getElementById('meta').innerHTML = mediacentre;
        //		refresh();
    }
}

function saveTextAsFile() {
    var output = '',
        id, textFileAsBlob, downloadLink, fileName;
    //	for (var key in localStorage) {
    for (id = 0; id < maxStation; id++) {
        //	output = output+(localStorage[key])+'\n';
        output = output + (localStorage[id]) + '\n';
    }
    fileName = document.getElementById('filesave').value;
    if (fileName == "")
        alert("Пожалуйста, укажите имя файла.");
    else {
        textFileAsBlob = new Blob([output], { type: 'text/plain' }),
            downloadLink = document.createElement("a");
        downloadLink.style.display = "none";
        downloadLink.setAttribute("download", fileName);
        document.body.appendChild(downloadLink);

        if (window.navigator.msSaveOrOpenBlob)
            downloadLink.addEventListener("click", function () {
                window.navigator.msSaveBlob(textFileAsBlob, fileName);
            });
        else if ('URL' in window)
            downloadLink.href = window.URL.createObjectURL(textFileAsBlob);
        else
            downloadLink.href = window.webkitURL.createObjectURL(textFileAsBlob);
        downloadLink.click();
    }
}

function full() {
    if (document.getElementById('Full').checked) {
        if ((document.getElementById('not1').innerHTML == "") && (document.getElementById('not1').innerHTML == ""))
            document.getElementById('lnot1').style.display = "none";
        else document.getElementById('lnot1').style.display = "inline-block";

        if (document.getElementById('bitr').innerHTML == "")
            document.getElementById('lbitr').style.display = "none";
        else document.getElementById('lbitr').style.display = "inline-block";

        if (document.getElementById('descr').innerHTML == "")
            document.getElementById('ldescr').style.display = "none";
        else document.getElementById('ldescr').style.display = "inline-block";

        if (document.getElementById('genre').innerHTML == "")
            document.getElementById('lgenre').style.display = "none";
        else document.getElementById('lgenre').style.display = "inline-block";

    } else {
        document.getElementById('lnot1').style.display = "none";
        document.getElementById('lbitr').style.display = "none";
        document.getElementById('ldescr').style.display = "none";
        document.getElementById('lgenre').style.display = "none";
    }
}

function icyResp(arr) {
    document.getElementById('curst').innerHTML = arr["curst"].replace(/\\/g, "");
    var select = document.getElementById('stationsSelect');
    var opt = select.options, i;
    for (i = 0; i < maxStation; i++) {
        var id = opt[i].value.split(":");
        id = id[0];
        if (id == document.getElementById('curst').innerHTML) break;
    }
    if (i == maxStation) i = 0;
    select.selectedIndex = i;
    if ((arr["descr"] == "") || (!document.getElementById('Full').checked))
        document.getElementById('ldescr').style.display = "none";
    else document.getElementById('ldescr').style.display = "inline-block";
    document.getElementById('descr').innerHTML = arr["descr"].replace(/\\/g, "");
    document.getElementById('name').innerHTML = arr["name"].replace(/\\/g, "");
    if ((arr["bitr"] == "") || (!document.getElementById('Full').checked)) { document.getElementById('lbitr').style.display = "none"; } else document.getElementById('lbitr').style.display = "inline-block";
    document.getElementById('bitr').innerHTML = arr["bitr"].replace(/\\/g, "") + " кБ/с";
    if (arr["bitr"] == "") document.getElementById('bitr').innerHTML = "";
    if (((arr["not1"] == "") && (arr["not2"] == "")) || (!document.getElementById('Full').checked)) document.getElementById('lnot1').style.display = "none";
    else document.getElementById('lnot1').style.display = "inline-block";
    document.getElementById('not1').innerHTML = arr["not1"].replace(/\\/g, "").replace(/^<BR>/, "");
    document.getElementById('not2').innerHTML = arr["not2"].replace(/\\/g, "");
    if ((arr["genre"] == "") || (!document.getElementById('Full').checked))
        document.getElementById('lgenre').style.display = "none";
    else document.getElementById('lgenre').style.display = "inline-block";
    document.getElementById('genre').innerHTML = arr["genre"].replace(/\\/g, "");
    if (arr["url1"] == "") {
        document.getElementById('lurl').style.display = "none";
        document.getElementById('icon').style.display = "none";
    } else {
        document.getElementById('lurl').style.display = "inline-block";
        document.getElementById('icon').style.display = "inline-block";
        var $url = arr["url1"].replace(/\\/g, "").replace(/ /g, "");
        if ($url == 'http://www.icecast.org/')
            document.getElementById('icon').src = "/logo.png";
        else document.getElementById('icon').src = "http://www.google.com/s2/favicons?domain_url=" + $url;
    }
    $url = arr["url1"].replace(/\\/g, "");
    document.getElementById('url1').innerHTML = $url;
    document.getElementById('url2').href = $url;
    if (arr["meta"] == "") {
        document.getElementById('meta').innerHTML = mediacentre;
    }
    if (arr["meta"]) document.getElementById('meta').innerHTML = arr["meta"].replace(/\\/g, "");
    changeTitle(document.getElementById('meta').innerHTML);
    if (typeof arr["auto"] != 'undefined') // undefined for websocket
        if (arr["auto"] == "1")
            document.getElementById("aplay").setAttribute("checked", "");
        else
            document.getElementById("aplay").removeAttribute("checked");
}

function soundResp(arr) {
    document.getElementById('vol_range').value = arr["vol"].replace(/\\/g, "");
    document.getElementById('treble_range').value = arr["treb"].replace(/\\/g, "");
    document.getElementById('bass_range').value = arr["bass"].replace(/\\/g, "");
    document.getElementById('treblefreq_range').value = arr["tfreq"].replace(/\\/g, "");
    document.getElementById('bassfreq_range').value = arr["bfreq"].replace(/\\/g, "");
    document.getElementById('spacial_range').value = arr["spac"].replace(/\\/g, "");
    onRangeVolChange(document.getElementById('vol_range').value, false);
    onRangeChange('treble_range', 'treble_span', 1.5, false, true);
    onRangeChange('bass_range', 'bass_span', 1, false, true);
    onRangeChangeFreqTreble('treblefreq_range', 'treblefreq_span', 1, false, true);
    onRangeChangeFreqBass('bassfreq_range', 'bassfreq_span', 10, false, true);
    onRangeChangeSpatial('spacial_range', 'spacial_span', true);
}

function refresh() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            try {
                var arr = JSON.parse(xhr.responseText);
                icyResp(arr);
                soundResp(arr);
            } catch (e) { console.log("error" + e); }
        }
    }
    try {
        xhr.open("POST", "icy", false);
        xhr.setRequestHeader(content, cjson);
        xhr.send();
        websocket.send("monitor");
    } catch (e) { console.log("error" + e); }
}

function resetEQ() {
    document.getElementById('treble_range').value = 0;
    onRangeChange('treble_range', 'treble_span', 1.5, false);
    document.getElementById('bass_range').value = 0;
    onRangeChange('bass_range', 'bass_span', 1, false);
    document.getElementById('treblefreq_range').value = 1;
    onRangeChangeFreqTreble('treblefreq_range', 'treblefreq_span', 1, false);
    document.getElementById('bassfreq_range').value = 2;
    onRangeChangeFreqBass('bassfreq_range', 'bassfreq_span', 10, false);
    document.getElementById('spacial_range').value = 0;
    onRangeChangeSpatial('spacial_range', 'spacial_span');
}

function onRangeChange($range, $spanid, $mul, $rotate, $nosave) {
    var val = document.getElementById($range).value;
    if ($rotate) val = document.getElementById($range).max - val;
    document.getElementById($spanid).innerHTML = (val * $mul) + " дБ";
    if (typeof ($nosave) == 'undefined') saveSoundSettings();
}

function onRangeChangeFreqTreble($range, $spanid, $mul, $rotate, $nosave) {
    var val = document.getElementById($range).value;
    if ($rotate) val = document.getElementById($range).max - val;
    document.getElementById($spanid).innerHTML = "От " + (val * $mul) + " кГц";
    if (typeof ($nosave) == 'undefined') saveSoundSettings();
}

function onRangeChangeFreqBass($range, $spanid, $mul, $rotate, $nosave) {
    var val = document.getElementById($range).value;
    if ($rotate) val = document.getElementById($range).max - val;
    document.getElementById($spanid).innerHTML = "До " + (val * $mul) + " Гц";
    if (typeof ($nosave) == 'undefined') saveSoundSettings();
}

function onRangeChangeSpatial($range, $spanid, $nosave) {
    var val = document.getElementById($range).value,
        label;
    switch (val) {
        case '0':
            label = "Выкл";
            break;
        case '1':
            label = "Минимум";
            break;
        case '2':
            label = "Средняя";
            break;
        case '3':
            label = "Максимум";
            break;
    }
    document.getElementById($spanid).innerHTML = label;
    if (typeof ($nosave) == 'undefined') saveSoundSettings();
}

function btnTDA($name, $step) {
    var val = parseInt(document.getElementById($name + '_range').value, 10) + $step;
    switch ($name) {
        case 'Volume':
            if (val < 0) val = 0;
            if (val > 17) val = 17;
            break;
        case 'Treble':
        case 'Bass':
            if (val < 0) val = 0;
            if (val > 14) val = 14;
            break;
        case 'AttLF':
        case 'AttRF':
        case 'AttLR':
        case 'AttRR':
            if (val < 0) val = 0;
            if (val > 13) val = 13;
            break;
        case 'sla1':
        case 'sla2':
        case 'sla3':
            if (val < 0) val = 0;
            if (val > 3) val = 3;
            break;
    }
    document.getElementById($name + '_range').value = val;
    onTDAchange($name);
}

function onTDAchange($name, $nosave) {
    var val = document.getElementById($name + '_range').value,
        label;
    switch ($name) {
        case 'Volume':
            switch (val) {
                case '0':
                    label = "-78.75";
                    break;
                case '1':
                    label = "-56.25";
                    break;
                case '2':
                    label = "-47.50";
                    break;
                case '3':
                    label = "-41.25";
                    break;
                case '4':
                    label = "-36.25";
                    break;
                case '5':
                    label = "-30.00";
                    break;
                case '6':
                    label = "-25.00";
                    break;
                case '7':
                    label = "-22.50";
                    break;
                case '8':
                    label = "-18.75";
                    break;
                case '9':
                    label = "-16.25";
                    break;
                case '10':
                    label = "-12.50";
                    break;
                case '11':
                    label = "-10.00";
                    break;
                case '12':
                    label = "-7.50";
                    break;
                case '13':
                    label = "-6.25";
                    break;
                case '14':
                    label = "-5.00";
                    break;
                case '15':
                    label = "-3.75";
                    break;
                case '16':
                    label = "-1.25";
                    break;
                case '17':
                    label = " 0";
                    break;
            };
            break;
        case 'Treble':
        case 'Bass':
            switch (val) {
                case '0':
                    label = "-14";
                    break;
                case '1':
                    label = "-12";
                    break;
                case '2':
                    label = "-10";
                    break;
                case '3':
                    label = "-8";
                    break;
                case '4':
                    label = "-6";
                    break;
                case '5':
                    label = "-4";
                    break;
                case '6':
                    label = "-2";
                    break;
                case '7':
                    label = " 0";
                    break;
                case '8':
                    label = "+2";
                    break;
                case '9':
                    label = "+4";
                    break;
                case '10':
                    label = "+6";
                    break;
                case '11':
                    label = "+8";
                    break;
                case '12':
                    label = "+10";
                    break;
                case '13':
                    label = "+12";
                    break;
                case '14':
                    label = "+14";
                    break;
            };
            break;
        case 'AttLF':
        case 'AttRF':
        case 'AttLR':
        case 'AttRR':
            switch (val) {
                case '0':
                    label = "-36.25";
                    break;
                case '1':
                    label = "-30";
                    break;
                case '2':
                    label = "-25";
                    break;
                case '3':
                    label = "-22.5";
                    break;
                case '4':
                    label = "-18.75";
                    break;
                case '5':
                    label = "-16.25";
                    break;
                case '6':
                    label = "-12.5";
                    break;
                case '7':
                    label = "-10";
                    break;
                case '8':
                    label = "-7.5";
                    break;
                case '9':
                    label = "-6.25";
                    break;
                case '10':
                    label = "-5";
                    break;
                case '11':
                    label = "-3.75";
                    break;
                case '12':
                    label = "-1.25";
                    break;
                case '13':
                    label = " 0";
                    break;
            };
            break;
        case 'sla1':
        case 'sla2':
        case 'sla3':
            switch (val) {
                case '0':
                    label = " 0";
                    break;
                case '1':
                    label = "+3.75";
                    break;
                case '2':
                    label = "+7.5";
                    break;
                case '3':
                    label = "+11.25";
                    break;
            };
            break;
    }
    document.getElementById($name + '_span').innerHTML = label + " дБ";
    if (typeof ($nosave) == 'undefined') hardware(1);
}

function logValue(value) {
    //Log(128/(Midi Volume + 1)) * (-10) * (Max dB below 0/(-24.04))
    var log = Number(value) + 1;
    var val = Math.round((Math.log10(255 / log) * 105.54571334));
    return val;
}
function VolumeBtn($element, $step) {
    var Volume = parseInt(document.getElementById($element).value, 10) + $step;
    if (Volume < 5) Volume = 0;
    if (Volume > 249) Volume = 254;
    onRangeVolChange(Volume, true);

}
function ToneBtn($element, $step) {
    var value = parseInt(document.getElementById($element).value, 10) + $step;
    if ($element != "spacial_range")
        document.getElementById($element).value = value;
    if ($element == "treble_range") {
        if (value < -8) value = -8;
        if (value > 7) value = 7;
        document.getElementById('treble_span').innerHTML = (value * 1.5) + " дБ";
    }
    if ($element == "treblefreq_range") {
        if (value < 1) value = 1;
        if (value > 15) value = 15;
        document.getElementById('treblefreq_span').innerHTML = "От " + value + " кГц";
    }
    if ($element == "bass_range") {
        if (value < 0) value = 0;
        if (value > 15) value = 15;
        document.getElementById('bass_span').innerHTML = value + " дБ";
    }
    if ($element == "bassfreq_range") {
        if (value < 2) value = 2;
        if (value > 15) value = 15;
        document.getElementById('bassfreq_span').innerHTML = "До " + (value * 10) + " Гц";
    }
    if ($element == "spacial_range") {
        if (value < 0) value = 0;
        if (value > 3) value = 3;
        document.getElementById($element).value = value;
        onRangeChangeSpatial('spacial_range', 'spacial_span', false);
    }
    saveSoundSettings();

}



function onRangeVolChange($value, $local) {
    var value = logValue($value);
    document.getElementById('vol1_span').innerHTML = (value * -0.5) + " дБ";
    document.getElementById('vol_span').innerHTML = (value * -0.5) + " дБ";
    document.getElementById('vol_range').value = $value;
    document.getElementById('vol1_range').value = $value;
    if ($local) {
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "soundvol", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send("vol=" + $value + "&");
    }
}

function wifi(valid) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var arr = JSON.parse(xhr.responseText);
            document.getElementById('ssid').value = arr["ssid"];
            document.getElementById('passwd').value = arr["pasw"];
            document.getElementById('ssid2').value = arr["ssid2"];
            document.getElementById('passwd2').value = arr["pasw2"];
            document.getElementById('ip').value = arr["ip"];
            chkip(document.getElementById('ip'));
            document.getElementById('mask').value = arr["msk"];
            chkip(document.getElementById('mask'));
            document.getElementById('gw').value = arr["gw"];
            chkip(document.getElementById('gw'));
            document.getElementById('ip2').value = arr["ip2"];
            chkip(document.getElementById('ip2'));
            document.getElementById('mask2').value = arr["msk2"];
            chkip(document.getElementById('mask2'));
            document.getElementById('gw2').value = arr["gw2"];
            chkip(document.getElementById('gw2'));
            document.getElementById('ua').value = arr["ua"];
            document.getElementById('host').value = arr["host"];
            if (arr["dhcp"] == "1")
                document.getElementById("dhcp").setAttribute("checked", "");
            else
                document.getElementById("dhcp").removeAttribute("checked");
            if (arr["dhcp2"] == "1")
                document.getElementById("dhcp2").setAttribute("checked", "");
            else
                document.getElementById("dhcp2").removeAttribute("checked");
            document.getElementById('Mac').innerHTML = arr["mac"];
            clickdhcp();
        }
    }
    xhr.open("POST", "wifi", false);
    xhr.setRequestHeader(content, ctype);
    xhr.send("valid=" + valid +
        "&ssid=" + encodeURIComponent(document.getElementById('ssid').value) +
        "&pasw=" + encodeURIComponent(document.getElementById('passwd').value) +
        "&ssid2=" + encodeURIComponent(document.getElementById('ssid2').value) +
        "&pasw2=" + encodeURIComponent(document.getElementById('passwd2').value) +
        "&ip=" + document.getElementById('ip').value +
        "&msk=" + document.getElementById('mask').value +
        "&gw=" + document.getElementById('gw').value +
        "&ip2=" + document.getElementById('ip2').value +
        "&msk2=" + document.getElementById('mask2').value +
        "&gw2=" + document.getElementById('gw2').value +
        "&ua=" + encodeURIComponent(document.getElementById('ua').value) +
        "&host=" + encodeURIComponent(document.getElementById('host').value) +
        "&dhcp=" + document.getElementById('dhcp').checked +
        "&dhcp2=" + document.getElementById('dhcp2').checked + "&");
}

function clickrear($nosave) {
    if (document.getElementById("rearON").checked)
        document.getElementById("Rear").style.display = "table";
    else
        document.getElementById("Rear").style.display = "none";
    if (typeof ($nosave) == 'undefined') hardware(1);
}

function clickloud() {
    hardware(1);
}

function mute() {
    hardware(1);
}

function loadCSV() {
    alert("Ещё не реализовано...");
}

function saveCSV() {
    alert("Ещё не реализовано...");
}

function control(save) {
    var set_control = "", set_option;
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var arr = JSON.parse(xhr.responseText);
            document.getElementById('LCD_BRG').value = arr["lcd_brg"];
        }
    }
    if (save == 0) {
        displaybright();
        FanControl();
    }
    else {
        set_option = "&ESPLOG=" + document.getElementById('ESPLOG').value +
            "&NTP=" + document.getElementById('NTP').value +
            "&NTP0=" + document.getElementById('NTP0').value +
            "&NTP1=" + document.getElementById('NTP1').value +
            "&NTP2=" + document.getElementById('NTP2').value +
            "&NTP3=" + document.getElementById('NTP3').value +
            "&TZONE=" + document.getElementById('TZONE').value +
            "&TIME_FORMAT=" + document.getElementById('TIME_FORMAT').value +
            "&LCD_ROTA=" + document.getElementById('LCD_ROTA').value;
        save = 0;
        set_option = "";
    }
    xhr.open("POST", "control", false);
    xhr.setRequestHeader(content, ctype);
    xhr.send("save=" + save +
        set_control +
        "&");
}

function devoptions(save) {
    var set_option = "";
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var arr = JSON.parse(xhr.responseText);
            document.getElementById('ESPLOG').value = arr["ESPLOG"];
            document.getElementById('NTP').value = arr["NTP"];
            document.getElementById('NTP0').value = arr["NTP0"];
            document.getElementById('NTP1').value = arr["NTP1"];
            document.getElementById('NTP2').value = arr["NTP2"];
            document.getElementById('NTP3').value = arr["NTP3"];
            document.getElementById('TZONE').value = arr["TZONE"];
            document.getElementById('TIME_FORMAT').value = arr["TIME_FORMAT"];
            document.getElementById('LCD_ROTA').value = arr["LCD_ROTA"];
        }
    }
    if (save == 0) {
        NTPservers();
        TimeZone();
    }
    else if ((save == 1) && (confirm("Сохранить настройки в NVS?\nДля вступления в силу некоторых настроек, Радиола может быть перезагружена."))) {
        set_option = "&ESPLOG=" + document.getElementById('ESPLOG').value +
            "&NTP=" + document.getElementById('NTP').value +
            "&NTP0=" + document.getElementById('NTP0').value +
            "&NTP1=" + document.getElementById('NTP1').value +
            "&NTP2=" + document.getElementById('NTP2').value +
            "&NTP3=" + document.getElementById('NTP3').value +
            "&TZONE=" + document.getElementById('TZONE').value +
            "&TIME_FORMAT=" + document.getElementById('TIME_FORMAT').value +
            "&LCD_ROTA=" + document.getElementById('LCD_ROTA').value;
    } else {
        save = 0;
        set_option = "";
    }
    xhr.open("POST", "devoptions", false);
    xhr.setRequestHeader(content, ctype);
    xhr.send("save=" + save +
        set_option +
        "&");
}

function FanControl() {
    if (document.getElementById('FanControl').value == 0) {
        document.getElementById("not_control").style.display = "none";
    } else {
        document.getElementById("not_control").style.display = "contents";
    }
    if (document.getElementById('FanControl').value == 1) {
        document.getElementById("by_auto").style.display = "contents";
        document.getElementById("by_handpwm").style.display = "none";

    } else {
        document.getElementById("by_handpwm").style.display = "contents";
        document.getElementById("by_auto").style.display = "none";
    }
}


function NTPservers() {
    if (document.getElementById('NTP').value == "0") {
        document.getElementById("custom_NTP").style.display = "none";
    } else {
        document.getElementById("custom_NTP").style.display = "contents";
    }

}

function TimeZone() {
    if (document.getElementById('TZONE').value == "CUSTOMTZ") {
        document.getElementById("edit_TZ").style.display = "contents";
    } else {
        document.getElementById("edit_TZ").style.display = "none";
    }

}

function displaybright() {
    if (document.getElementById('LCD_BRG').value == 0) {
        document.getElementById("brightOn").style.display = "none";
    } else {
        document.getElementById("brightOn").style.display = "";
        if (document.getElementById('LCD_BRG').value == 1) {
            document.getElementById("by_hand").style.visibility = "collapse";
            document.getElementById("by_time").style.visibility = "visible";
            document.getElementById("_night").style.visibility = "visible";
            document.getElementById("brg").style.visibility = "visible";
            document.getElementById("brg_sun").style.visibility = "visible";
            document.getElementById("by_lighting").style.visibility = "collapse";
        }
        if (document.getElementById('LCD_BRG').value == 2) {
            document.getElementById("by_hand").style.visibility = "collapse";
            document.getElementById("by_time").style.visibility = "collapse";
            document.getElementById("_night").style.visibility = "collapse";
            document.getElementById("brg").style.visibility = "visible";
            document.getElementById("brg_sun").style.visibility = "visible";
            document.getElementById("by_lighting").style.visibility = "visible";
        }
        if (document.getElementById('LCD_BRG').value == 3) {
            document.getElementById("by_hand").style.visibility = "visible";
            document.getElementById("_night").style.visibility = "collapse";
            document.getElementById("by_time").style.visibility = "collapse";
            document.getElementById("brg").style.visibility = "collapse";
            document.getElementById("brg_sun").style.visibility = "collapse";
            document.getElementById("by_lighting").style.visibility = "collapse";
        }
    }
}

function gpios(gpio_mode, save, load) {
    var setgpios = "",
        gpio_mode_txt, err, style_color, xhr;
    xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var arr = JSON.parse(xhr.responseText);
            if (load == 1) {
                gpio_mode = parseInt(arr["GPIO_MODE"].replace(/\\/g, ""), 10);
            }
            err = arr["ERROR"].replace(/\\/g, "");
            if (err != "0000") {
                gpio_mode_txt = _ERR + err + "<br>";
                if (err == "1102")
                    gpio_mode_txt = gpio_mode_txt + _noNVS;
                style_color = "red";
            } else {
                if (gpio_mode == 0) {
                    gpio_mode_txt = _default;
                    style_color = "blue";
                } else {
                    if (save == 0) {
                        gpio_mode_txt = _loadNVS;
                    } else {
                        gpio_mode_txt = _saveNVS;
                    }
                    style_color = "green";
                }
            }
            document.getElementById("GPIOsErr").style.color = style_color;
            document.getElementById("GPIOsErr").innerHTML = gpio_mode_txt;
            document.getElementById("K_SPI").innerHTML = parseInt(arr["K_SPI"].replace(/\\/g, ""), 10);
            document.getElementById("P_MISO").innerHTML = parseInt(arr["P_MISO"].replace(/\\/g, ""), 10);
            document.getElementById("P_MOSI").innerHTML = parseInt(arr["P_MOSI"].replace(/\\/g, ""), 10);
            document.getElementById("P_CLK").innerHTML = parseInt(arr["P_CLK"].replace(/\\/g, ""), 10);
            document.getElementById("P_XCS").innerHTML = parseInt(arr["P_XCS"].replace(/\\/g, ""), 10);
            document.getElementById("P_XDCS").innerHTML = parseInt(arr["P_XDCS"].replace(/\\/g, ""), 10);
            document.getElementById("P_DREQ").innerHTML = parseInt(arr["P_DREQ"].replace(/\\/g, ""), 10);
            document.getElementById("P_ENC0_A").innerHTML = parseInt(arr["P_ENC0_A"].replace(/\\/g, ""), 10);
            document.getElementById("P_ENC0_B").innerHTML = parseInt(arr["P_ENC0_B"].replace(/\\/g, ""), 10);
            document.getElementById("P_ENC0_BTN").innerHTML = parseInt(arr["P_ENC0_BTN"].replace(/\\/g, ""), 10);
            document.getElementById("P_I2C_SCL").innerHTML = parseInt(arr["P_I2C_SCL"].replace(/\\/g, ""), 10);
            document.getElementById("P_I2C_SDA").innerHTML = parseInt(arr["P_I2C_SDA"].replace(/\\/g, ""), 10);
            document.getElementById("P_LCD_CS").innerHTML = parseInt(arr["P_LCD_CS"].replace(/\\/g, ""), 10);
            document.getElementById("P_LCD_A0").innerHTML = parseInt(arr["P_LCD_A0"].replace(/\\/g, ""), 10);
            document.getElementById("P_IR_SIGNAL").innerHTML = parseInt(arr["P_IR_SIGNAL"].replace(/\\/g, ""), 10);
            document.getElementById("P_BACKLIGHT").innerHTML = parseInt(arr["P_BACKLIGHT"].replace(/\\/g, ""), 10);
            document.getElementById("P_TACHOMETER").innerHTML = parseInt(arr["P_TACHOMETER"].replace(/\\/g, ""), 10);
            document.getElementById("P_FAN_SPEED").innerHTML = parseInt(arr["P_FAN_SPEED"].replace(/\\/g, ""), 10);
            document.getElementById("P_DS18B20").innerHTML = parseInt(arr["P_DS18B20"].replace(/\\/g, ""), 10);
            document.getElementById("P_TOUCH_CS").innerHTML = parseInt(arr["P_TOUCH_CS"].replace(/\\/g, ""), 10);
            document.getElementById("P_BUZZER").innerHTML = parseInt(arr["P_BUZZER"].replace(/\\/g, ""), 10);
            document.getElementById("P_RXD").innerHTML = parseInt(arr["P_RXD"].replace(/\\/g, ""), 10);
            document.getElementById("P_TXD").innerHTML = parseInt(arr["P_TXD"].replace(/\\/g, ""), 10);
            document.getElementById("P_LDR").innerHTML = parseInt(arr["P_LDR"].replace(/\\/g, ""), 10);
        }
    }
    if ((save == 1) && (confirm("Записать зачения GPIO в NVS?"))) {
        //		if (confirm("Записать зачения GPIO в NVS?")) {
        //		alert("Перезагрузка Радиолы. Пожалуйста, подождите.");
        setgpios = "&save=" + save +
            "&K_SPI=" + document.getElementById("K_SPI").innerHTML +
            "&P_MISO=" + document.getElementById("P_MISO").innerHTML +
            "&P_MOSI=" + document.getElementById("P_MOSI").innerHTML +
            "&P_CLK=" + document.getElementById("P_CLK").innerHTML +
            "&P_XCS=" + document.getElementById("P_XCS").innerHTML +
            "&P_XDCS=" + document.getElementById("P_XDCS").innerHTML +
            "&P_DREQ=" + document.getElementById("P_DREQ").innerHTML +
            "&P_ENC0_A=" + document.getElementById("P_ENC0_A").innerHTML +
            "&P_ENC0_B=" + document.getElementById("P_ENC0_B").innerHTML +
            "&P_ENC0_BTN=" + document.getElementById("P_ENC0_BTN").innerHTML +
            "&P_I2C_SCL=" + document.getElementById("P_I2C_SCL").innerHTML +
            "&P_I2C_SDA=" + document.getElementById("P_I2C_SDA").innerHTML +
            "&P_LCD_CS=" + document.getElementById("P_LCD_CS").innerHTML +
            "&P_LCD_A0=" + document.getElementById("P_LCD_A0").innerHTML +
            "&P_IR_SIGNAL=" + document.getElementById("P_IR_SIGNAL").innerHTML +
            "&P_BACKLIGHT=" + document.getElementById("P_BACKLIGHT").innerHTML +
            "&P_TACHOMETER=" + document.getElementById("P_TACHOMETER").innerHTML +
            "&P_FAN_SPEED=" + document.getElementById("P_FAN_SPEED").innerHTML +
            "&P_DS18B20=" + document.getElementById("P_DS18B20").innerHTML +
            "&P_TOUCH_CS=" + document.getElementById("P_TOUCH_CS").innerHTML +
            "&P_BUZZER=" + document.getElementById("P_BUZZER").innerHTML +
            "&P_RXD=" + document.getElementById("P_RXD").innerHTML +
            "&P_TXD=" + document.getElementById("P_TXD").innerHTML +
            "&P_LDR=" + document.getElementById("P_LDR").innerHTML;
    } else save = 0;
    xhr.open("POST", "gpios", false);
    xhr.setRequestHeader(content, ctype);
    xhr.send("gpio_mode=" + gpio_mode +
        setgpios +
        "&");
}

function hardware(save) {
    var i, inputnum, loud = 0, xhr,
        mute = 1,
        rear = 0,
        sendsave = "";
    xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var arr = JSON.parse(xhr.responseText);
            if (arr['present'] == "1") {
                tabbis.onClick(1, parseInt(arr['inputnum'], 10) - 1);
                switch (arr['inputnum']) {
                    case '1':
                        document.getElementById('source').innerHTML = "AUX";
                        break;
                    case '2':
                        document.getElementById('source').innerHTML = "VS1053";
                        break;
                    case '3':
                        document.getElementById('source').innerHTML = "BT201";
                        break;
                }
                document.getElementById('Volume_range').value = arr["Volume"].replace(/\\/g, "");
                onTDAchange('Volume', false);
                document.getElementById('Treble_range').value = arr["Treble"].replace(/\\/g, "");
                onTDAchange('Treble', false);
                document.getElementById('Bass_range').value = arr["Bass"].replace(/\\/g, "");
                onTDAchange('Bass', false);
                if (arr["rear_on"] == "1") {
                    document.getElementById('rearON').setAttribute("checked", "");
                    document.getElementById('AttLR_range').value = arr["attlr"].replace(/\\/g, "");
                    onTDAchange('AttLR', false);
                    document.getElementById('AttRR_range').value = arr["attrr"].replace(/\\/g, "");
                    onTDAchange('AttRR', false);
                } else
                    document.getElementById('rearON').removeAttribute("checked");
                clickrear(false);
                document.getElementById('AttLF_range').value = arr["attlf"].replace(/\\/g, "");
                onTDAchange('AttLF', false);
                document.getElementById('AttRF_range').value = arr["attrf"].replace(/\\/g, "");
                onTDAchange('AttRF', false);

                for (i = 1; i < 4; i++) {
                    if (arr["loud" + i] == "1")
                        document.getElementById('loud' + i).setAttribute("checked", "");
                    else
                        document.getElementById('loud' + i).removeAttribute("checked");
                    document.getElementById('sla' + i + '_range').value = arr["sla" + i].replace(/\\/g, "");
                    onTDAchange('sla' + i, false);
                }
                if (arr["mute"] == "0")
                    document.getElementById('mute').setAttribute("checked", "");
                else
                    document.getElementById('mute').removeAttribute("checked");
            } else {
                document.getElementById("barTDA7313").style.display = "none";
                document.getElementById("barTDA7313radio").style.display = "none";
                //                document.getElementById("audioinput1").style.display = "none";
                //                document.getElementById("audioinput3").style.display = "none";
                tabbis.onClick(1, 1);
                document.getElementById("soudBar").style.pointerEvents = "none";
            }
        }
    }
    if (document.getElementById("barTDA7313").style.display == "") {
        for (inputnum = 1; inputnum < 4; inputnum++)
            if (document.getElementById('audioinput' + inputnum).className == "active") break;
        if (inputnum == 4) inputnum = 1;

        if (document.getElementById('rearON').checked) rear = 1;
        if (document.getElementById('loud' + inputnum).checked) loud = 1;
        if (document.getElementById('mute').checked) mute = 0;
        sendsave = "&inputnum=" + inputnum +
            "&Volume=" + document.getElementById('Volume_range').value +
            "&Treble=" + document.getElementById('Treble_range').value +
            "&Bass=" + document.getElementById('Bass_range').value +
            "&rear_on=" + rear +
            "&attlf=" + document.getElementById('AttLF_range').value +
            "&attrf=" + document.getElementById('AttRF_range').value +
            "&attlr=" + document.getElementById('AttLR_range').value +
            "&attrr=" + document.getElementById('AttRR_range').value +
            "&loud=" + loud +
            "&sla=" + document.getElementById('sla' + inputnum + '_range').value +
            "&mute=" + mute;
    } else save = 0;
    xhr.open("POST", "hardware", false);
    xhr.setRequestHeader(content, ctype);
    xhr.send("save=" + save +
        sendsave +
        "&");

}





function instantPlay() {
    var curl, xhr;
    try {
        xhr = new XMLHttpRequest();
        xhr.open("POST", "instant_play", false);
        xhr.setRequestHeader(content, ctype);
        curl = document.getElementById('instant_path').value;
        if (!(curl.substring(0, 1) === "/")) curl = "/" + curl;
        document.getElementById('instant_url').value = document.getElementById('instant_url').value.replace(/^https?:\/\//, '');
        curl = fixedEncodeURIComponent(curl);
        xhr.send("url=" + document.getElementById('instant_url').value + "&port=" + document.getElementById('instant_port').value + "&path=" + curl + "&");
    } catch (e) { console.log("error" + e); }
}

function buildAddURL() {
    if (document.getElementById('add_port').value == "")
        document.getElementById('add_port').value = "80";
    if (document.getElementById('add_path').value == "")
        document.getElementById('add_path').value = "/";
    document.getElementById('add_URL').value = "http://" + document.getElementById('add_url').value + ':' +
        document.getElementById('add_port').value + document.getElementById('add_path').value;
}

function buildURL() {
    if (document.getElementById('instant_port').value == "")
        document.getElementById('instant_port').value = "80";
    if (document.getElementById('instant_path').value == "")
        document.getElementById('instant_path').value = "/";
    document.getElementById('instant_URL').value = "http://" + document.getElementById('instant_url').value + ':' +
        document.getElementById('instant_port').value + document.getElementById('instant_path').value;
}

function parseURL() {
    var a = document.createElement('a');
    a.href = document.getElementById('instant_URL').value;
    if (a.hostname != location.hostname)
        document.getElementById('instant_url').value = a.hostname;
    else {
        document.getElementById('instant_URL').value = "http://" + document.getElementById('instant_URL').value;
        a.href = document.getElementById('instant_URL').value;
        document.getElementById('instant_url').value = a.hostname;
    }
    if (a.port == "")
        document.getElementById('instant_port').value = "80";
    else document.getElementById('instant_port').value = a.port;
    document.getElementById('instant_path').value = a.pathname + a.search + a.hash;
}

function prevStation() {
    var select = document.getElementById('stationsSelect').selectedIndex;
    if (select > 0) {
        document.getElementById('stationsSelect').selectedIndex = select - 1;
        Select();
    }
}

function nextStation() {
    var select = document.getElementById('stationsSelect').selectedIndex;
    if (select < document.getElementById('stationsSelect').length - 1) {
        document.getElementById('stationsSelect').selectedIndex = select + 1;
        Select();
    }
}
// autoplay checked or unchecked
function autoplay() {
    var xhr;
    xhr = new XMLHttpRequest();
    try {
        xhr.open("POST", "auto", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send("id=" + document.getElementById('aplay').checked + "&");
    } catch (e) { console.log("error" + e); }
}

//ask for the state of autoplay
function autostart() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var arr = JSON.parse(xhr.responseText);
            if (arr["rauto"] == "1")
                document.getElementById("aplay").setAttribute("checked", "");
            else
                document.getElementById("aplay").removeAttribute("checked");
        }
    }
    try {
        xhr.open("POST", "rauto", false); // request auto state
        xhr.setRequestHeader(content, ctype);
        xhr.send("&");
    } catch (e) { console.log("error" + e); }
}

function Select() {
    if (document.getElementById('aplay').checked)
        playStation();
}

function setEditBackground(tr) {
    tr.style.background = "darkblue";
    tr.style.color = "greenyellow";
}

function playEditStation(tr) {
    if (stchanged) stChanged();
    var id = tr.cells[0].innerText;
    if ((editPlaying) && (editIndex == tr)) {
        stopStation();
        tr.style.background = "initial";
        tr.style.color = "initial";
        //		editPlaying = false;
        editIndex = 0;
    } else {
        if (editIndex != 0) {
            editIndex.style.background = "initial";
            editIndex.style.color = "initial";
        }
        wsplayStation(id); // select the station in the list
        //		editPlaying = true;
        editIndex = tr;
        setEditBackground(tr);
        playStation(); //play it 
    }
}

function playStation() {
    var select, id, xhr;
    try {
        //	checkwebsocket();
        mpause();
        select = document.getElementById('stationsSelect');
        id = select.options[select.selectedIndex].value.split(":");
        id = id[0];
        localStorage.setItem('selindexstore', select.selectedIndex.toString());
        xhr = new XMLHttpRequest();
        xhr.open("POST", "play", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send("id=" + id + "&");
        editPlaying = true;
    } catch (e) { console.log("error" + e); }
}

function stopStation() {
    var select = document.getElementById('stationsSelect'), xhr;
    //	checkwebsocket();
    mstop();
    editPlaying = false;
    localStorage.setItem('selindexstore', select.options.selectedIndex.toString());
    try {
        xhr = new XMLHttpRequest();
        xhr.open("POST", "stop", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send();
    } catch (e) { console.log("error" + e); }
}

function saveSoundSettings() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "sound", false);
    xhr.setRequestHeader(content, ctype);
    xhr.send(
        "&bass=" + document.getElementById('bass_range').value +
        "&treble=" + document.getElementById('treble_range').value +
        "&bassfreq=" + document.getElementById('bassfreq_range').value +
        "&treblefreq=" + document.getElementById('treblefreq_range').value +
        "&spacial=" + document.getElementById('spacial_range').value +
        "&");
}

function fixedEncodeURIComponent(str) {
    return str.replace(/[&]/g, function (c) {
        return '%' + c.charCodeAt(0).toString(16);
    });
}

function saveStation() {
    var file = document.getElementById('add_path').value,
        url = document.getElementById('add_url').value,
        jfile, xhr;
    if (!(file.substring(0, 1) === "/")) file = "/" + file;
    console.log("Path: " + file);
    jfile = fixedEncodeURIComponent(file);
    console.log("JSON: " + jfile);
    url = url.replace(/^https?:\/\//, '');
    try {
        xhr = new XMLHttpRequest();
        xhr.open("POST", "setStation", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send("nb=" + 1 + "&id=" + document.getElementById('add_slot').value + "&url=" + url + "&name=" + document.getElementById('add_name').value + "&file=" + jfile + "&port=" + document.getElementById('add_port').value + "&&");
        localStorage.setItem(document.getElementById('add_slot').value, "{\"Name\":\"" + document.getElementById('add_name').value + "\",\"URL\":\"" + url + "\",\"File\":\"" + file + "\",\"Port\":\"" + document.getElementById('add_port').value + "\"}");
    } catch (e) { console.log("error save " + e); }
    abortStation(); // to erase the edit field
    loadStations();
    loadStationsList(maxStation);
}

function abortStation() {
    document.getElementById('editStationDiv').style.display = "none";
}

function eraseStation() {
    document.getElementById('editStationDiv').style.display = "block";
    document.getElementById('add_url').value = "";
    document.getElementById('add_name').value = "";
    document.getElementById('add_path').value = "";
    document.getElementById('add_port').value = "80";
    document.getElementById('add_URL').value = ""
}

//
function editInstantStation() {
    var arr, id = 0;

    do {
        var idstr = id.toString();
        try {
            arr = JSON.parse(localStorage.getItem(idstr));
        } catch (e) { console.log("error" + e); }
        id++;
    } while (arr["URL"].length > 0);

    id--;

    if (id <= 254) {
        document.getElementById('editStationDiv').style.display = "block";
        document.getElementById('add_slot').value = id;

        document.getElementById('add_URL').value = "http://" + document.getElementById('instant_url').value + ":" + document.getElementById('instant_port').value + document.getElementById('instant_path').value;
        parseEditURL();
    } else alert("Нет свободного слота.");
}


function editStation(id) {
    var arr, xhr;
    document.getElementById('editStationDiv').style.display = "block";

    function cpedit(arr) {

        document.getElementById('add_url').value = arr["URL"];
        document.getElementById('add_name').value = arr["Name"];
        document.getElementById('add_path').value = arr["File"];
        if (arr["Port"] == "0") arr["Port"] = "80";
        document.getElementById('add_port').value = arr["Port"];
        document.getElementById('add_URL').value = "http://" + document.getElementById('add_url').value + ":" + document.getElementById('add_port').value + document.getElementById('add_path').value;
    }
    document.getElementById('add_slot').value = id;
    var idstr = id.toString();
    if (localStorage.getItem(idstr) != null) {

        try {
            arr = JSON.parse(localStorage.getItem(idstr));
        } catch (e) { console.log("error" + e); }
        cpedit(arr);
    } else {
        xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () {
            if (xhr.readyState == 4 && xhr.status == 200) {
                try {
                    arr = JSON.parse(xhr.responseText);
                } catch (e) { console.log("error" + e); }
                cpedit(arr);
            }
        }
        xhr.open("POST", "getStation", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send("idgp=" + id + "&");
    }
}


function parseEditURL() {
    var a = document.createElement('a');
    a.href = document.getElementById('add_URL').value;
    if (a.hostname != location.hostname)
        document.getElementById('add_url').value = a.hostname;
    else {
        document.getElementById('add_URL').value = "http://" + document.getElementById('add_URL').value;
        a.href = document.getElementById('add_URL').value;
        document.getElementById('add_url').value = a.hostname;
    }
    if (a.port == "")
        document.getElementById('add_port').value = "80";
    else document.getElementById('add_port').value = a.port;
    document.getElementById('add_path').value = a.pathname + a.search + a.hash;
}

function refreshList() {
    promptworking(working);
    intervalid = window.setTimeout(refreshListtemp, 10);
}

function refreshListtemp() {
    if (stchanged) stChanged();
    localStorage.clear();
    loadStationsList(maxStation);
    loadStations();

    promptworking("");
    refresh();
}

function clearList() {
    promptworking(working);
    if (confirm("Предупреждение: это сотрет все станции. Обязательно сохраните станции раньше. Стереть сейчас?")) {
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "clear", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send();
        refreshList();
        window.setTimeout(loadStations, 10);
    } else promptworking("");
}

function ircode(mode) {
    var xhr;
    try {
        xhr = new XMLHttpRequest();
        xhr.open("POST", "ircode", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send("mode=" + mode + "&");
    } catch (e) { console.log("error" + e); }

}

function upgrade() {
    var xhr;
    try {
        xhr = new XMLHttpRequest();
        xhr.open("POST", "upgrade", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send();
    } catch (e) { console.log("error" + e); }

    //	alert("Rebooting to the new release\nPlease refresh the page in few seconds.");
}
function getversion() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var arr = JSON.parse(xhr.responseText);
            document.getElementById('currentrelease').innerHTML = arr["RELEASE"].replace(/\\/g, "") + " Rev: " + arr["REVISION"].replace(/\\/g, "");
            if (arr["curtemp"] != "000.0") {
                document.getElementById('curtemp').innerHTML = parseFloat(arr["curtemp"]) + ' ℃';
                document.getElementById('_temp').style.display = "block";
            }
            if (arr["rpmfan"] != "0000") {
                document.getElementById('rpmfan').innerHTML = arr["rpmfan"] + ' Об/м';
                document.getElementById('_fan').style.display = "block";
            }
        }
    }
    try {
        xhr.open("POST", "version", false);
        xhr.setRequestHeader(content, ctype);
        xhr.send("&");
    } catch (e) { console.log("error" + e); }
}

function checkhistory() {
    var xhr;
    if (window.XDomainRequest) {
        xhr = new XDomainRequest();
    } else if (window.XMLHttpRequest) {
        xhr = new XMLHttpRequest();
    }
    xhr.onload = function () {
        document.getElementById('History').innerHTML = xhr.responseText;
    }
    xhr.open("GET", "https://serverdoma.ru/esp32/history132.php", false);
    try {
        xhr.send(null);
    } catch (e) { ; }
}

function checkinfos() {
    var xhr;
    if (window.XDomainRequest) {
        xhr = new XDomainRequest();
    } else if (window.XMLHttpRequest) {
        xhr = new XMLHttpRequest();
    }
    xhr.onload = function () {
        document.getElementById('Infos').innerHTML = xhr.responseText;
    }
    xhr.open("GET", "https://serverdoma.ru/esp32/infos.php", false);
    try {
        xhr.send(null);
    } catch (e) { ; }
}
//load the version and history html
function checkversion() {
    var xhr;
    getversion();
    if (window.XDomainRequest) {
        xhr = new XDomainRequest();
    } else if (window.XMLHttpRequest) {
        xhr = new XMLHttpRequest();
    }
    xhr.onload = function () {
        document.getElementById('Version').innerHTML = xhr.responseText;
        document.getElementById('newrelease').innerHTML = document.getElementById('firmware_last').innerHTML;
    }
    xhr.open("GET", "https://serverdoma.ru/esp32/version32.php", false);
    try {
        xhr.send(null);
    } catch (e) { ; }
    checkinfos();
    //checkhistory();
}

// refresh the stations list by reading a file
function downloadStations() {
    var i, indmax, tosend, arr, reader, lines, line, file, xhr;
    if (window.File && window.FileReader && window.FileList && window.Blob) {
        reader = new FileReader();
        xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () {
            promptworking(working); // some time to display promptworking
        }
        reader.onload = function (e) {
            function fillInfo(ind, arri) {
                tosend = tosend + "&id=" + ind + "&url=" + arri["URL"] + "&name=" + arri["Name"] + "&file=" + fixedEncodeURIComponent(arri["File"]) + "&port=" + arri["Port"] + "&";
                localStorage.setItem(ind, "{\"Name\":\"" + arri["Name"] + "\",\"URL\":\"" + arri["URL"] + "\",\"File\":\"" + arri["File"] + "\",\"Port\":\"" + arri["Port"] + "\"}");
            }
            // Entire file
            //console.log(this.result);
            // By lines
            lines = this.result.split('\n');
            localStorage.clear();
            indmax = 3;
            line = 0;
            try {
                tosend = "nb=" + indmax;
                for (i = 0; i < indmax; i++) {
                    arr = JSON.parse(lines[i]);
                    fillInfo(i, arr);
                }
                xhr.open("POST", "setStation", false);
                xhr.setRequestHeader(content, ctype);
                //				console.log("post "+tosend);
                xhr.send(tosend);
            } catch (e) { console.log("error " + e + " " + tosend); }
            //			}
            indmax = 2;
            for (line = 3; line < lines.length; line += indmax) {
                //				console.log(lines[line]);
                try {
                    tosend = "nb=" + indmax;
                    for (i = 0; i < indmax; i++) {
                        arr = JSON.parse(lines[line + i]);
                        fillInfo(line + i, arr);
                    }
                    xhr.open("POST", "setStation", false);
                    xhr.setRequestHeader(content, ctype);
                    xhr.send(tosend);
                } catch (e) { console.log("error " + e + " " + tosend); }
            }
            loadStationsList(maxStation);

        };
        file = document.getElementById('fileload').files[0];
        if (file == null) alert("Пожалуйста, выберите файл.");
        else {
            promptworking(working);
            reader.readAsText(file);
        }
    }
}

function dragStart(ev) {
    ev.dataTransfer.setData("Text", ev.target.id);
}

function moveNodes(a, b) {
    var pa1 = a.parentNode,
        sib = b.nextSibling,
        txt;
    if (sib === a) sib = sib.nextSibling;
    pa1.insertBefore(a, b);
    txt = 0;
    for (txt = 0; txt < maxStation; txt++) {
        pa1.rows[txt].cells[0].innerText = txt.toString();
        pa1.rows[txt].cells[3].innerHTML = b.parentNode.rows[txt].cells[3].innerHTML;
    }
    stchanged = true;
    document.getElementById("stsave").disabled = false;
}

function dragDrop(ev) {
    ev.preventDefault();
    var TRStart = document.getElementById(ev.dataTransfer.getData("text"));
    var TRDrop = document.getElementById(ev.currentTarget.id);
    moveNodes(TRStart, TRDrop);
}

function allowDrop(ev) {
    ev.preventDefault();
}

function stChanged() {
    var i, xhr, indmax, tosend, index, tbody = document.getElementById("stationsTable").getElementsByTagName('tbody')[0];

    function fillInfo(ind) {
        var parser = document.createElement('a');

        var id = tbody.rows[ind].cells[0].innerText;
        var name = tbody.rows[ind].cells[1].innerText;
        var furl = tbody.rows[ind].cells[2].innerText;
        parser.href = "http://" + furl;
        var url = parser.hostname;
        var file = parser.pathname + parser.hash + parser.search;
        var port = parser.port;
        if (!port) port = 80;
        /*				file=tbody.rows[ind].cells[3].innerText;
                        port= tbody.rows[ind].cells[4].innerText;*/
        localStorage.setItem(id, "{\"Name\":\"" + name + "\",\"URL\":\"" + url + "\",\"File\":\"" + file + "\",\"Port\":\"" + port + "\"}");
        tosend = tosend + "&id=" + id + "&url=" + url + "&name=" + name + "&file=" + file + "&port=" + port + "&";
    }
    promptworking(working); // some time to display promptworking
    if (stchanged && confirm("Список изменен. Вы хотите сохранить измененный список?")) {
        xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () { }
        promptworking(working); // some time to display promptworking
        localStorage.clear();
        indmax = 7;
        index = 0; {
            try {
                tosend = "nb=" + indmax;
                for (i = 0; i < indmax; i++)
                    fillInfo(index + i);
                xhr.open("POST", "setStation", false);
                xhr.setRequestHeader(content, ctype);
                xhr.send(tosend);
            } catch (e) { console.log("error " + e); }
        }
        indmax = 8;
        for (index = 7; index < maxStation; index += indmax) {
            try {
                tosend = "nb=" + indmax;
                for (i = 0; i < indmax; i++)
                    fillInfo(index + i);
                xhr.open("POST", "setStation", false);
                xhr.setRequestHeader(content, ctype);
                xhr.send(tosend);
            } catch (e) { console.log("error " + e); }
        }
        loadStationsList(maxStation);
    } else
        if (stchanged)
            loadStations();
    stchanged = false;
    document.getElementById("stsave").disabled = true;
    promptworking("");
}
//Load the Stations table
function loadStations() {
    var new_tbody = document.createElement('tbody'),
        idlist, idstr, select,
        id, xhr, arr;

    function cploadStations(id, arr) {
        var tr = document.createElement('TR'),
            td = document.createElement('TD');
        tr.draggable = "true";
        tr.id = "tr" + id.toString();
        tr.ondragstart = dragStart;
        tr.ondrop = dragDrop;
        tr.ondragover = allowDrop;
        td.appendChild(document.createTextNode(id));
        td.style.paddingLeft = "3px;";
        td.setAttribute('onclick', 'playEditStation(this.parentNode);');
        tr.appendChild(td);
        // Name
        td = document.createElement('TD');
        td.style.wordBreak = "break-all";
        td.style.overflowWrap = "break-word";
        td.style.wordWrap = "break-word";
        td.setAttribute('onclick', 'playEditStation(this.parentNode);');
        if (arr["Name"].length > 116) arr["Name"] = "Error";
        td.appendChild(document.createTextNode(arr["Name"]));
        tr.appendChild(td);
        // URL
        td = document.createElement('TD');
        td.style.wordBreak = "break-all";
        td.style.overflowWrap = "break-word";
        td.style.wordWrap = "break-word";
        td.setAttribute('onclick', 'playEditStation(this.parentNode);');
        if (arr["Name"].length > 0) {
            if (arr["URL"].length > 116) arr["URL"] = "Error";
            if (arr["File"].length > 116) arr["File"] = "Error";
            if (arr["Port"].length > 116) arr["Port"] = "Error";
            td.appendChild(document.createTextNode(arr["URL"] + ":" + arr["Port"] + arr["File"]));
        } else
            td.appendChild(document.createTextNode(""));
        tr.appendChild(td);

        // edit button			
        td = document.createElement('TD');
        td.style.textAlign = "center";
        td.style.verticalAlign = "middle";
        td.innerHTML = "<a class=\"icon-page-edit\" href=\"javascript:void(0)\" onClick=\"editStation(" + id + ")\"></a>";
        tr.appendChild(td);
        if (idlist === idstr) {
            setEditBackground(tr);
            editIndex = tr;
            editPlaying = true;
        }
        new_tbody.appendChild(tr);
    }
    select = document.getElementById('stationsSelect');
    try {
        idlist = select.options[select.selectedIndex].value.split(":");
        idlist = idlist[0];
    } catch (e) { idlist = 0; }
    for (id = 0; id < maxStation; id++) {
        idstr = id.toString();
        if (localStorage.getItem(idstr) != null) {
            try {
                arr = JSON.parse(localStorage.getItem(idstr));
            } catch (e) { console.log("error" + e); }
            cploadStations(id, arr);
        } else
            try {
                xhr = new XMLHttpRequest();
                xhr.onreadystatechange = function () {
                    if (xhr.readyState == 4 && xhr.status == 200) {
                        try {
                            arr = JSON.parse(xhr.responseText);
                        } catch (e) { console.log("error" + e); }
                        localStorage.setItem(idstr, xhr.responseText);
                        cploadStations(id, arr);
                    }
                }
                xhr.open("POST", "getStation", false);
                xhr.setRequestHeader(content, ctype);
                xhr.send("idgp=" + id + "&");
            } catch (e) {
                console.log("error" + e);
                id--;
            }
    }

    var old_tbody = document.getElementById("stationsTable").getElementsByTagName('tbody')[0];
    old_tbody.parentNode.replaceChild(new_tbody, old_tbody);
}

//Load the selection with all stations
function loadStationsList(max) {
    var foundNull = false,
        id, arr, select, i, xhr;
    select = document.getElementById("stationsSelect");
    for (i = select.options.length - 1; i >= 0; i--) {
        select.remove(i);
    }

    function cploadStationsList(id, arr) {
        foundNull = false;
        if (arr["Name"].length > 0) {
            var opt = document.createElement('option');
            opt.appendChild(document.createTextNode(id + ": \t" + arr["Name"]));
            select.add(opt);
        } else foundNull = true;
        return foundNull;
    }
    select.disabled = true;
    promptworking(working);
    for (id = 0; id < max; id++) {
        var idstr = id.toString();
        if (localStorage.getItem(idstr) != null) {
            try {
                arr = JSON.parse(localStorage.getItem(idstr));
            } catch (e) { console.log("error" + e); }
            foundNull = cploadStationsList(id, arr);
        } else
            try {
                xhr = new XMLHttpRequest();
                xhr.onreadystatechange = function () {
                    if (xhr.readyState == 4 && xhr.status == 200) {
                        try {
                            arr = JSON.parse(xhr.responseText);
                        } catch (e) { console.log("error" + e); }
                        localStorage.setItem(idstr, xhr.responseText);
                        foundNull = cploadStationsList(id, arr);
                    }
                }
                xhr.open("POST", "getStation", false);
                xhr.setRequestHeader(content, ctype);
                xhr.send("idgp=" + id + "&");
            } catch (e) {
                console.log("error" + e);
                id--;
            }
    }

    promptworking("");
    select.disabled = false;
    select.options.selectedIndex = parseInt(localStorage.getItem('selindexstore'), 10);
}

function printList() {
    var html = "<html>";
    var id, arr;
    html += "</html>";
    html += "<h1>Список станций Радиолы</h1><br/><hr><br/>";
    for (id = 0; id < maxStation; id++) {
        var idstr = id.toString();
        if (localStorage.getItem(idstr) != null) {
            try {
                arr = JSON.parse(localStorage.getItem(idstr));
            } catch (e) { console.log("error" + e); }
            if (arr["Name"].length > 0) {
                html += idstr + "&nbsp;&nbsp;" + arr["Name"] + "<br/>";
            }
        }
    }
    var printWin = window.open('', '', 'left=0,top=0,width=1,height=1,toolbar=0,scrollbars=0,status  =0');
    printWin.document.write(html);
    printWin.document.close();
    printWin.focus();
    printWin.print();
    printWin.close();
}

document.addEventListener("DOMContentLoaded", function () {
    localStorage.removeItem("tabbis");
    tabbis.init();
    document.getElementById("PLAYER").addEventListener("click", function () {
        if (stchanged) stChanged();
        refresh();
        curtab = "PLAYER";
    });
    document.getElementById("STATIONS").addEventListener("click", function () {
        if (stchanged) stChanged();
        loadStations();
        curtab = "STATIONS";
    });
    document.getElementById("SOUND").addEventListener("click", function () {
        if (stchanged) stChanged();
        curtab = "SOUND";
        hardware(0);
    });
    document.getElementById("SETTINGS").addEventListener("click", function () {
        if (stchanged) stChanged();
        curtab = "SETTINGS";
        document.getElementById("GPIOS").addEventListener("click", function () {
            gpios(0, 0, 1);
        });
        document.getElementById("REMOTE").addEventListener("click", function () {
            ircodes(0, 0, 1);
        });
        document.getElementById("WIFI").addEventListener("click", function () {
            wifi(0);
        });
        document.getElementById("OPTIONS").addEventListener("click", function () {
            devoptions(0);
            control(0);
        });
        document.getElementById("UPDATE").addEventListener("click", function () {
            checkversion();
        });
    });
    window.addEventListener("keydown", function (event) {
        if (event.defaultPrevented) {
            return;
        }
        if (event.ctrlKey) {
            switch (event.key) {
                case ' ':
                    if (editPlaying) stopStation();
                    else playStation();
                    break;
                case "ArrowDown":
                    nextStation();
                    break;
                case "ArrowLeft":
                    var vol = parseInt(document.getElementById('vol_range').value, 10);
                    if (vol < 5) vol = 0;
                    else vol = vol - 5;
                    onRangeVolChange(vol, true);
                    break;
                case "ArrowUp":
                    prevStation();
                    break;
                case "ArrowRight":
                    var vol = parseInt(document.getElementById('vol_range').value, 10);
                    if (vol > 249) vol = 254;
                    else vol = vol + 5;
                    onRangeVolChange(vol, true);
                    break;
                default:
                    return;
            }
            event.preventDefault();
        }
        if (curtab == "STATIONS") {
            var ed = editPlaying;
            loadStations();
            editPlaying = ed;
        }
    }, true);

    if (intervalid != 0) window.clearTimeout(intervalid);
    intervalid = 0;
    if (timeid != 0) window.clearInterval(timeid);
    timeid = window.setInterval(dtime, 1000);
    if (window.location.hostname != "192.168.4.1") {
        loadStationsList(maxStation);
    }
    else {
        document.getElementById("WIFI").click();
        tabbis.onClick(0, 3);
        tabbis.onClick(2, 0);
    }
    checkwebsocket();
    promptworking("");
    refresh();
    wifi(0);
    hardware(0);
    autostart();
    checkversion();
    consoleON();
});
//ScrollUp
function ScrollUp() {
    var t, s;
    s = document.body.scrollTop || window.pageYOffset;
    t = setInterval(function () { if (s > 0) window.scroll(0, s -= 5); else clearInterval(t) }, 5);
}
// 
// 