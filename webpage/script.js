const mediacentre = "Радиола",
    COMMON = "common",
    FAVORITES = "favorites",
    maxStation = 128,
    working = "Работаю... Пожалуйста, подождите.",
    _ERR = "ОШИБКА ", _default = "ПО УМОЛЧАНИЮ", _loadNVS = "СЧИТАНО ИЗ NVS", _saveNVS = "ЗАПИСАНО В NVS", _noNVS = "Записей в NVS ещё нет!!!",
    AUX = 1, VS1053 = 2, BT201 = 3;

var auto, intervalid, timeid, websocket, urlmonitor, monitor, e, tblEvn,
    moniPlaying = false,
    editPlaying = false,
    current_total = 0,
    PlayList = COMMON,
    audioinput = AUX,
    editIndex = 0,
    curtab = "PLAYER",
    stchanged = false,
    RadiolaStorage = {
        "common": [],
        "common_select": 0,
        "favorites": [],
        "favorites_select": 0
    };
function openwebsocket() {
    websocket = new WebSocket("ws://" + window.location.host + "/");
    // console.log("url:" + "ws://" + window.location.host + "/");


    websocket.onmessage = function (event) {
        try {
            var arr = JSON.parse(event.data);
            // console.log("onmessage:" + event.data);
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
                document.getElementById('loc_url').value = arr["iurl"];
                buildURL();
            }
            if (arr["ipath"]) {
                document.getElementById('loc_path').value = arr["ipath"];
                buildURL();
            }
            if (arr["iport"]) {
                document.getElementById('loc_port').value = arr["iport"];
                buildURL();
            }
            if (arr["wsrssi"]) {
                document.getElementById('rssi').innerHTML = arr["wsrssi"] + ' дБм';
            }
            if (arr["wscurtemp"]) {
                document.getElementById('curtemp').innerHTML = arr["wscurtemp"] + ' ℃';
            }
            if (arr["wsrpmfan"]) {
                document.getElementById('rpmfan').innerHTML = arr["wsrpmfan"] + ' Об/м';
            }
        } catch (e) { console.log("err: " + e); }
    }

    websocket.onopen = function (event) {
        console.log("Open, url:" + "ws://" + window.location.host + "/");
        if (window.timerID) { /* a setInterval has been fired */
            window.clearInterval(window.timerID);
            window.timerID = 0;
        }
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
function changeTitle(arr) {
    window.parent.document.title = arr; // on change l'attribut <title>
}

function wsplayStation(arg) {
    var i;
    var select = document.getElementById('select_' + PlayList);
    for (i = 0; i < select.options.length; i++) {
        var id = select.options[i].value;
        if (id == arg) break;
    }

    select.selectedIndex = i;
}

function playMonitor(arr) {
    urlmonitor = "";
    urlmonitor = arr;
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

function mvol(name, step) {
    var val = parseInt(document.getElementById(name + '_range').value, 10) + step;
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    document.getElementById(name + '_range').value = val;
    document.getElementById(name + '_span').innerHTML = val + "%";
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
function chkip(ip) {
    if (/^([0-9]+\.){3}[0-9]+$/.test(ip.value)) ip.style.color = "green";
    else ip.style.color = "red";
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
        if (document.getElementById('current_key').innerHTML == val) {
            checkir = true;
        }
        if (window.checkir == true) {
            alert("Такой код уже назначен!");
            document.getElementById('new_key').innerHTML = "";
        }
        else {
            document.getElementById('K_' + num).innerHTML = val;
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
    var setircodes = "", ir_mode_txt, err, style_color;
    var arr = JSON.parse(PostData("ircodes", "ir_mode=" + ir_mode + "&load=" + load));
    if (load) {
        ir_mode = parseInt(arr["IR_MODE"], 10);
    }
    err = arr["ERROR"];
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
    if (!save) {
        for (var i = 0; i < 17; i++) {
            document.getElementById('K_' + i).innerHTML = arr["K_" + i];
        }
    }
    if ((save == 1) && (confirm("Записать коды пульта в NVS?"))) {
        for (var i = 0; i < 17; i++) {
            setircodes += "&K_" + i + "=" + document.getElementById('K_' + i).innerHTML;
        }
        PostData("ircodes", "ir_mode=" + ir_mode + "&save=" + save + setircodes);
    } else save = 0;

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
    }
}

function savePlayList(PLS) {
    var output = '#EXTM3U\n',
        id, port, textFileAsBlob, downloadLink, fileName;
    for (id = 0; id < RadiolaStorage[PLS].length; id++) {
        try {
            var obj = RadiolaStorage[PLS][id];
            if (obj["Port"] == "80") {
                port = '';
            } else {
                port = ':' + obj["Port"];
            }
            output = output + '#EXTINF:-1,' + obj["Name"] + '\n';
            output = output + 'http://' + obj["URL"] + port + obj["File"] + '\n';
        } catch (e) {

            console.log("err: " + e);

        }
    }
    fileName = document.getElementById('filename_' + PLS).value;
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
    var curSt = parseInt(arr["curst"], 10) + 1;
    document.getElementById('curst').innerHTML = curSt;
    var select = document.getElementById('select_' + COMMON);
    var i;
    for (i = 0; i < select.options.length; i++) {
        var id = select.options[i].value;
        if (id === arr["curst"]) break;
    }
    if (i == select.options.length) i = 0;
    select.selectedIndex = i;

    select = document.getElementById('select_' + FAVORITES);
    for (i = 0; i < select.options.length; i++) {
        if (RadiolaStorage[FAVORITES][i]["ID"] == arr["curst"])
            break;
    }
    if (i == select.options.length) i = 0;
    select.selectedIndex = i;


    if ((arr["descr"] == "") || (!document.getElementById('Full').checked))
        document.getElementById('ldescr').style.display = "none";
    else document.getElementById('ldescr').style.display = "inline-block";
    document.getElementById('descr').innerHTML = arr["descr"].replace(/\\/g, "");
    document.getElementById('name').innerHTML = arr["name"].replace(/\\/g, "");
    if ((arr["bitr"] == "") || (!document.getElementById('Full').checked)) {
        document.getElementById('lbitr').style.display = "none";
    } else { document.getElementById('lbitr').style.display = "inline-block"; }
    document.getElementById('bitr').innerHTML = arr["bitr"].replace(/\\/g, "");
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
        var url = arr["url1"].replace(/\\/g, "").replace(/ /g, "");
        if (url == 'http://www.icecast.org/')
            document.getElementById('icon').src = "/logo.png";
        else document.getElementById('icon').src = "http://www.google.com/s2/favicons?domain_url=" + url;
    }
    url = arr["url1"].replace(/\\/g, "");
    document.getElementById('url1').innerHTML = url;
    document.getElementById('url2').href = url;
    if (arr["meta"] == "") {
        document.getElementById('meta').innerHTML = mediacentre;
    }
    if (arr["meta"]) document.getElementById('meta').innerHTML = arr["meta"].replace(/\\/g, "");
    changeTitle(document.getElementById('meta').innerHTML);
    if (typeof arr["auto"] != 'undefined') // undefined for websocket
        if (arr["auto"] == "1") {
            // document.getElementById("aplay").setAttribute("checked", "");
            document.getElementById("auto_common").setAttribute("checked", "");
        }
        else {
            // document.getElementById("aplay").removeAttribute("checked");
            document.getElementById("auto_common").removeAttribute("checked");
        }
}

function soundResp(arr) {
    document.getElementById('vol_range').value = arr["vol"];
    document.getElementById('treble_range').value = arr["treb"];
    document.getElementById('bass_range').value = arr["bass"];
    document.getElementById('treblefreq_range').value = arr["tfreq"];
    document.getElementById('bassfreq_range').value = arr["bfreq"];
    document.getElementById('spacial_range').value = arr["spac"];
    onRangeVolChange(document.getElementById('vol_range').value, false);
    onRangeChange('treble_range', 'treble_span', 1.5, false, true);
    onRangeChange('bass_range', 'bass_span', 1, false, true);
    onRangeChangeFreqTreble('treblefreq_range', 'treblefreq_span', 1, false, true);
    onRangeChangeFreqBass('bassfreq_range', 'bassfreq_span', 10, false, true);
    onRangeChangeSpatial('spacial_range', 'spacial_span', true);
}

function refresh() {
    var arr = JSON.parse(PostData("icy"));
    icyResp(arr);
    soundResp(arr);

    try {
        websocket.send("monitor");
    } catch (e) { console.log("err: " + e); }
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

function onRangeChange(range, spanid, mul, rotate, nosave) {
    var val = document.getElementById(range).value;
    if (rotate) val = document.getElementById(range).max - val;
    document.getElementById(spanid).innerHTML = (val * mul) + " дБ";
    if (typeof (nosave) == 'undefined') saveSoundSettings();
}

function onRangeChangeFreqTreble(range, spanid, mul, rotate, nosave) {
    var val = document.getElementById(range).value;
    if (rotate) val = document.getElementById(range).max - val;
    document.getElementById(spanid).innerHTML = "От " + (val * mul) + " кГц";
    if (typeof (nosave) == 'undefined') saveSoundSettings();
}

function onRangeChangeFreqBass(range, spanid, mul, rotate, nosave) {
    var val = document.getElementById(range).value;
    if (rotate) val = document.getElementById(range).max - val;
    document.getElementById(spanid).innerHTML = "До " + (val * mul) + " Гц";
    if (typeof (nosave) == 'undefined') saveSoundSettings();
}

function onRangeChangeSpatial(range, spanid, nosave) {
    var val = document.getElementById(range).value,
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
    document.getElementById(spanid).innerHTML = label;
    if (typeof (nosave) == 'undefined') saveSoundSettings();
}

function btnTDA(name, step) {
    var val = parseInt(document.getElementById(name + '_range').value, 10) + step;
    switch (name) {
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
    document.getElementById(name + '_range').value = val;
    onTDAchange(name);
}

function onTDAchange(name, nosave) {
    var val = document.getElementById(name + '_range').value,
        label;
    switch (name) {
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
    document.getElementById(name + '_span').innerHTML = label + " дБ";
    if (typeof (nosave) == 'undefined') hardware(1);
}

function logValue(value) {
    //Log(128/(Midi Volume + 1)) * (-10) * (Max dB below 0/(-24.04))
    var log = Number(value) + 1;
    var val = Math.round((Math.log10(255 / log) * 105.54571334));
    return val;
}
function VolumeBtn(element, step) {
    var Volume = parseInt(document.getElementById(element).value, 10) + step;
    if (Volume < 5) Volume = 0;
    if (Volume > 249) Volume = 254;
    onRangeVolChange(Volume, true);

}
function ToneBtn(element, step) {
    var value = parseInt(document.getElementById(element).value, 10) + step;
    if (element != "spacial_range")
        document.getElementById(element).value = value;
    if (element == "treble_range") {
        if (value < -8) value = -8;
        if (value > 7) value = 7;
        document.getElementById('treble_span').innerHTML = (value * 1.5) + " дБ";
    }
    if (element == "treblefreq_range") {
        if (value < 1) value = 1;
        if (value > 15) value = 15;
        document.getElementById('treblefreq_span').innerHTML = "От " + value + " кГц";
    }
    if (element == "bass_range") {
        if (value < 0) value = 0;
        if (value > 15) value = 15;
        document.getElementById('bass_span').innerHTML = value + " дБ";
    }
    if (element == "bassfreq_range") {
        if (value < 2) value = 2;
        if (value > 15) value = 15;
        document.getElementById('bassfreq_span').innerHTML = "До " + (value * 10) + " Гц";
    }
    if (element == "spacial_range") {
        if (value < 0) value = 0;
        if (value > 3) value = 3;
        document.getElementById(element).value = value;
        onRangeChangeSpatial('spacial_range', 'spacial_span', false);
    }
    saveSoundSettings();

}



function onRangeVolChange(val, local) {
    var value = logValue(val);
    document.getElementById('vol1_span').innerHTML = (value * -0.5) + " дБ";
    document.getElementById('vol_span').innerHTML = (value * -0.5) + " дБ";
    document.getElementById('vol_range').value = val;
    document.getElementById('vol1_range').value = val;
    if (local) {
        PostData("soundvol", "vol=" + val + "&");
    }
}

function wifi(valid) {
    if (!valid) {
        var arr = JSON.parse(PostData("wifi"));
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
    } else {
        PostData("wifi", "valid=" + valid +
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
}

function clickrear(nosave) {
    if (document.getElementById("rearON").checked)
        document.getElementById("Rear").style.display = "table";
    else
        document.getElementById("Rear").style.display = "none";
    if (typeof (nosave) == 'undefined') hardware(1);
}

function clickloud() {
    hardware(1);
}

function mute() {
    hardware(1);
}

function control(save) {
    var set_control = "", set_option;

    if (save == 0) {
        var arr = JSON.parse(PostData("control"));
        document.getElementById('LCD_BRG').value = arr["lcd_brg"];
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
        PostData("control", "save=" + save + set_control + "&");
    }
}

function devoptions(save) {
    var set_option = "";
    if (save == 0) {
        var arr = JSON.parse(PostData("devoptions"));
        document.getElementById('ESPLOG').value = arr["ESPLOG"];
        document.getElementById('NTP').value = arr["NTP"];
        document.getElementById('NTP0').value = arr["NTP0"];
        document.getElementById('NTP1').value = arr["NTP1"];
        document.getElementById('NTP2').value = arr["NTP2"];
        document.getElementById('NTP3').value = arr["NTP3"];
        document.getElementById('TZONE').value = arr["TZONE"];
        document.getElementById('TIME_FORMAT').value = arr["TIME_FORMAT"];
        document.getElementById('LCD_ROTA').value = arr["LCD_ROTA"];



        NTPservers();
    } else if ((save == 1) && (confirm("Сохранить настройки в NVS?\nДля вступления в силу некоторых настроек, Радиола может быть перезагружена."))) {
        set_option = "&ESPLOG=" + document.getElementById('ESPLOG').value +
            "&NTP=" + document.getElementById('NTP').value +
            "&NTP0=" + document.getElementById('NTP0').value +
            "&NTP1=" + document.getElementById('NTP1').value +
            "&NTP2=" + document.getElementById('NTP2').value +
            "&NTP3=" + document.getElementById('NTP3').value +
            "&TZONE=" + document.getElementById('TZONE').value +
            "&TIME_FORMAT=" + document.getElementById('TIME_FORMAT').value +
            "&LCD_ROTA=" + document.getElementById('LCD_ROTA').value;
    } else if ((save == 2) && (confirm("Вернуть заводские настройки Радиолы?\nВсе сохранённые настройки в NVS будут удалены безвозвратно!!!"))) {
        set_option = "";
    } else {
        save = 0;
        set_option = "";
    }
    if (save != 0) {
        PostData("devoptions", "save=" + save +
            set_option +
            "&");
    }
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
        gpio_mode_txt, err, style_color;
    var arr = JSON.parse(PostData("gpios", "&gpio_mode=" + gpio_mode + "&load=" + load));
    if (load) {
        gpio_mode = parseInt(arr["GPIO_MODE"], 10);
    }
    err = arr["ERROR"];
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
    if (!save) {
        document.getElementById("GPIOsErr").style.color = style_color;
        document.getElementById("GPIOsErr").innerHTML = gpio_mode_txt;
        document.getElementById("P_MISO").innerHTML = parseInt(arr["P_MISO"], 10);
        document.getElementById("P_MOSI").innerHTML = parseInt(arr["P_MOSI"], 10);
        document.getElementById("P_CLK").innerHTML = parseInt(arr["P_CLK"], 10);
        document.getElementById("P_XCS").innerHTML = parseInt(arr["P_XCS"], 10);
        document.getElementById("P_XDCS").innerHTML = parseInt(arr["P_XDCS"], 10);
        document.getElementById("P_DREQ").innerHTML = parseInt(arr["P_DREQ"], 10);
        document.getElementById("P_ENC0_A").innerHTML = parseInt(arr["P_ENC0_A"], 10);
        document.getElementById("P_ENC0_B").innerHTML = parseInt(arr["P_ENC0_B"], 10);
        document.getElementById("P_ENC0_BTN").innerHTML = parseInt(arr["P_ENC0_BTN"], 10);
        document.getElementById("P_I2C_SCL").innerHTML = parseInt(arr["P_I2C_SCL"], 10);
        document.getElementById("P_I2C_SDA").innerHTML = parseInt(arr["P_I2C_SDA"], 10);
        document.getElementById("P_LCD_CS").innerHTML = parseInt(arr["P_LCD_CS"], 10);
        document.getElementById("P_LCD_A0").innerHTML = parseInt(arr["P_LCD_A0"], 10);
        document.getElementById("P_IR_SIGNAL").innerHTML = parseInt(arr["P_IR_SIGNAL"], 10);
        document.getElementById("P_BACKLIGHT").innerHTML = parseInt(arr["P_BACKLIGHT"], 10);
        document.getElementById("P_TACHOMETER").innerHTML = parseInt(arr["P_TACHOMETER"], 10);
        document.getElementById("P_FAN_SPEED").innerHTML = parseInt(arr["P_FAN_SPEED"], 10);
        document.getElementById("P_DS18B20").innerHTML = parseInt(arr["P_DS18B20"], 10);
        document.getElementById("P_TOUCH_CS").innerHTML = parseInt(arr["P_TOUCH_CS"], 10);
        document.getElementById("P_BUZZER").innerHTML = parseInt(arr["P_BUZZER"], 10);
        document.getElementById("P_RXD").innerHTML = parseInt(arr["P_RXD"], 10);
        document.getElementById("P_TXD").innerHTML = parseInt(arr["P_TXD"], 10);
        document.getElementById("P_LDR").innerHTML = parseInt(arr["P_LDR"], 10);
    }
    if ((save == 1) && (confirm("Записать зачения GPIO в NVS?"))) {

        setgpios = "&save=1" +
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
        PostData("gpios", "gpio_mode=" + gpio_mode +
            setgpios +
            "&");
    }
}

function hardware(save) {
    var i, loud = 0, mute = 1,
        rear = 0,
        sendsave = "";
    if (!save) {
        var arr = JSON.parse(PostData("hardware", "save=" + save + "&"));
        if (arr['present'] == "1") {
            switch (arr['audioinput']) {
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
            gotoTAB(document.getElementById('source').innerHTML);
            document.getElementById('Volume_range').value = arr["Volume"];
            onTDAchange('Volume', false);
            document.getElementById('Treble_range').value = arr["Treble"];
            onTDAchange('Treble', false);
            document.getElementById('Bass_range').value = arr["Bass"];
            onTDAchange('Bass', false);
            if (arr["rear_on"] == "1") {
                document.getElementById('rearON').setAttribute("checked", "");
                document.getElementById('AttLR_range').value = arr["attlr"];
                onTDAchange('AttLR', false);
                document.getElementById('AttRR_range').value = arr["attrr"];
                onTDAchange('AttRR', false);
            } else
                document.getElementById('rearON').removeAttribute("checked");
            clickrear(false);
            document.getElementById('AttLF_range').value = arr["attlf"];
            onTDAchange('AttLF', false);
            document.getElementById('AttRF_range').value = arr["attrf"];
            onTDAchange('AttRF', false);

            for (i = 1; i < 4; i++) {
                if (arr["loud" + i] == "1")
                    document.getElementById('loud' + i).setAttribute("checked", "");
                else
                    document.getElementById('loud' + i).removeAttribute("checked");
                document.getElementById('sla' + i + '_range').value = arr["sla" + i];
                onTDAchange('sla' + i, false);
            }
            if (arr["mute"] == "0")
                document.getElementById('mute').setAttribute("checked", "");
            else
                document.getElementById('mute').removeAttribute("checked");
        } else {
            document.getElementById("barTDA7313").style.display = "none";
            document.getElementById("barTDA7313radio").style.display = "none";
            gotoTAB("SOUND", "VS1053");
            document.getElementById("soudBar").style.pointerEvents = "none";
        }
    }

    if (document.getElementById("barTDA7313").style.display == "") {
        if (document.getElementById('rearON').checked) rear = 1;
        if (document.getElementById('loud' + audioinput).checked) loud = 1;
        if (document.getElementById('mute').checked) mute = 0;
        sendsave = "&audioinput=" + audioinput +
            "&Volume=" + document.getElementById('Volume_range').value +
            "&Treble=" + document.getElementById('Treble_range').value +
            "&Bass=" + document.getElementById('Bass_range').value +
            "&rear_on=" + rear +
            "&attlf=" + document.getElementById('AttLF_range').value +
            "&attrf=" + document.getElementById('AttRF_range').value +
            "&attlr=" + document.getElementById('AttLR_range').value +
            "&attrr=" + document.getElementById('AttRR_range').value +
            "&loud=" + loud +
            "&sla=" + document.getElementById('sla' + audioinput + '_range').value +
            "&mute=" + mute;
    } else save = 0;
    PostData("hardware", "save=" + save +
        sendsave +
        "&");

}





function LocalPlay() {
    var curl;
    try {

        curl = document.getElementById('loc_path').value;
        if (!(curl.substring(0, 1) === "/")) curl = "/" + curl;
        document.getElementById('loc_url').value = document.getElementById('loc_url').value.replace(/^https?:\/\//, '');
        curl = fixedEncodeURIComponent(curl);
        PostData("local_play", "url=" + document.getElementById('loc_url').value + "&port=" +
            document.getElementById('loc_port').value + "&path=" + curl + "&");
    } catch (e) { console.log("err: " + e); }
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
    if (document.getElementById('loc_port').value == "")
        document.getElementById('loc_port').value = "80";
    if (document.getElementById('loc_path').value == "")
        document.getElementById('loc_path').value = "/";
    document.getElementById('loc_URL').value = "http://" + document.getElementById('loc_url').value + ':' +
        document.getElementById('loc_port').value + document.getElementById('loc_path').value;
}


function parseURL() {
    var a = document.createElement('a');
    a.href = document.getElementById('loc_URL').value;
    if (a.hostname != location.hostname)
        document.getElementById('loc_url').value = a.hostname;
    else {
        document.getElementById('loc_URL').value = "http://" + document.getElementById('loc_URL').value;
        a.href = document.getElementById('loc_URL').value;
        document.getElementById('loc_url').value = a.hostname;
    }
    if (a.port == "")
        document.getElementById('loc_port').value = "80";
    else document.getElementById('loc_port').value = a.port;
    document.getElementById('loc_path').value = a.pathname + a.search + a.hash;
}


function prevStation(PlayList) {
    var select = document.getElementById('select_' + PlayList).selectedIndex;
    if (select > 0) {
        document.getElementById('select_' + PlayList).selectedIndex = select - 1;
        Select(PlayList);
    }
}

function nextStation(PlayList) {
    var select = document.getElementById('select_' + PlayList).selectedIndex;
    if (select < document.getElementById('select_' + PlayList).length - 1) {
        document.getElementById('select_' + PlayList).selectedIndex = select + 1;
        Select(PlayList);
    }
}
// autoplay checked or unchecked
function autoplay() {
    try {
        PostData("auto", "id=" + document.getElementById("auto_common").checked + "&");
    } catch (e) { console.log("err: " + e); }
}

//ask for the state of autoplay
function autostart() {
    var arr = JSON.parse(PostData("rauto"));
    if (arr["rauto"] == "1") {
        // document.getElementById("aplay").setAttribute("checked", "");
        document.getElementById("auto_common").setAttribute("checked", "");
    }
    else {
        // document.getElementById("aplay").removeAttribute("checked");
        document.getElementById("auto_common").removeAttribute("checked");
    }
    PostData("rauto");
}

function Select(PlayList) {
    if (document.getElementById('auto_' + PlayList).checked)
        playStation(PlayList);
}

function setEditBackground(tr) {
    tr.style.background = "darkblue";
    tr.style.color = "greenyellow";
}

function playEditStation(tr) {
    if (stchanged) {
        stChanged();
        return;
    }
    var id = (tr.cells[0].innerText) - 1;
    setPLS(tr);
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
        playStation(PlayList); //play it 
    }
}

function playStation(PlayList) {
    var select, id;
    try {
        mpause();
        select = document.getElementById("select_" + PlayList);
        id = select.options[select.selectedIndex].value;
        RadiolaStorage[PlayList + "_select"] = id;
        localStorage.setItem("RadiolaStorage", JSON.stringify(RadiolaStorage));
        editPlaying = true;
        if (PlayList == FAVORITES) {
            id = RadiolaStorage[FAVORITES][id]["ID"];
        }
        PostData("play", "id=" + id + "&");
    } catch (e) { console.log("err: " + e); }
}

function stopStation(PlayList) {
    var select = document.getElementById('select_' + PlayList);
    //	checkwebsocket();
    mstop();
    editPlaying = false;
    RadiolaStorage[PlayList + "_select"] = select.options.selectedIndex;
    localStorage.setItem("RadiolaStorage", JSON.stringify(RadiolaStorage));
    try {
        PostData("stop");
    } catch (e) { console.log("err: " + e); }
}

function saveSoundSettings() {
    PostData("sound",
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
        jfile, save = false,
        name = document.getElementById('add_name').value,
        port = document.getElementById('add_port').value;
    var id = document.getElementById('add_slot').value - 1;
    if (!(file.substring(0, 1) === "/")) file = "/" + file;
    jfile = fixedEncodeURIComponent(file);
    url = url.replace(/^https?:\/\//, '');
    try {
        if (id < RadiolaStorage[COMMON].length) {
            RadiolaStorage[COMMON][id].Name = name;
            RadiolaStorage[COMMON][id].URL = url;
            RadiolaStorage[COMMON][id].File = jfile;
            RadiolaStorage[COMMON][id].Port = port;
        } else {
            RadiolaStorage[COMMON].push({
                "Name": name,
                "URL": url,
                "File": jfile,
                "Port": port,
                "FAV": false
            });
            save = true;
        }

        abortStation();
    } catch (e) { console.log("error save " + e); }

    promptworking(working);
    var tosend;
    var fav = 0;
    if (RadiolaStorage[COMMON][id]["FAV"]) {
        fav = 1;
    }
    tosend = "&ID=" + id + "&url=" + RadiolaStorage[COMMON][id].URL + "&name=" + RadiolaStorage[COMMON][id].Name +
        "&file=" + fixedEncodeURIComponent(RadiolaStorage[COMMON][id].File) + "&port=" + RadiolaStorage[COMMON][id].Port + "&fav=" + fav;
    PostData("setStation", tosend);
    if (save)
        PostData("setStation", "&save=1");

    localStorage.setItem("RadiolaStorage", JSON.stringify(RadiolaStorage));
    refreshList();
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
function editLocalStation() {
    var id = RadiolaStorage[COMMON].length;
    if (id <= (maxStation - 1)) {
        document.getElementById('editStationDiv').style.display = "block";
        document.getElementById('editST').innerHTML = "Добавление станции";
        document.getElementById('add_slot').value = (id + 1);

        document.getElementById('add_URL').value = "http://" + document.getElementById('loc_url').value + ":" +
            document.getElementById('loc_port').value + document.getElementById('loc_path').value;
        parseEditURL();
    } else alert("Нет свободного слота.");
}

function editStation(num) {
    var arr;
    setPLS(num.parentNode);
    var id = (num.parentNode.cells[0].innerText) - 1;

    document.getElementById('editStationDiv').style.display = "block";
    document.getElementById('editST').innerHTML = "Редактирование станции";
    function cpedit(arr) {

        document.getElementById('add_url').value = arr["URL"];
        document.getElementById('add_name').value = arr["Name"];
        document.getElementById('add_path').value = arr["File"];
        if (arr["Port"] == "0") arr["Port"] = "80";
        document.getElementById('add_port').value = arr["Port"];
        document.getElementById('add_URL').value = "http://" + document.getElementById('add_url').value + ":" + document.getElementById('add_port').value +
            document.getElementById('add_path').value;
    }
    document.getElementById('add_slot').value = (id + 1);
    var idstr = id.toString();
    try {
        arr = RadiolaStorage[PlayList][idstr];
    } catch (e) { console.log("err: " + e); }
    cpedit(arr);
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

function removeStation(el) {
    if (!confirm("Вы действительно хотите удалить станцию из списка?")) {
        return;
    }
    var num = el.parentNode.children[0].id.split('_')[1];
    setPLS(el.parentNode);

    var table = document.getElementById("table_" + PlayList).getElementsByTagName('tbody')[0];
    table.deleteRow(num);
    for (var i = 0; i < table.rows.length; i++) {

        table.rows[i].cells[0].innerText = (i + 1).toString();
        table.rows[i].cells[0].id = 'num_' + i;
    }
    if (PlayList == FAVORITES) {
        var id = RadiolaStorage[FAVORITES][num]["ID"];
        RadiolaStorage[COMMON][id]["FAV"] = false;
    } else {
        RadiolaStorage[COMMON].splice(num, 1);
    }
    stchanged = true;
    document.getElementById("stsave").disabled = false;
}

function refreshList() {
    if (stchanged) stChanged();
    for (var i = 0; i < 2; i++) {
        setPLS(i);
        loadStations(PlayList);
        loadStationsList(PlayList);
        promptworking("");
    }

}

function clearList(storage) {
    var text;
    if (storage == FAVORITES) {
        text = 'ИЗБРАННОГО';
    } else {
        text = 'ОБЩЕГО';
    }
    if (confirm("Вы действительно хотите удалить плейлист из " + text + " списка?")) {
        promptworking(working);
        if (storage == COMMON) {
            PostData("clear");
            clearPlayList(FAVORITES);
        } else {
            for (var id = 0; id < RadiolaStorage[COMMON].length; id++) {
                if (RadiolaStorage[COMMON][id]["FAV"] == true) {
                    RadiolaStorage[COMMON][id]["FAV"] = false;
                    var tosend = "&ID=" + id + "&url=" + RadiolaStorage[COMMON][id].URL + "&name=" + RadiolaStorage[COMMON][id].Name +
                        "&file=" + fixedEncodeURIComponent(RadiolaStorage[COMMON][id].File) + "&port=" + RadiolaStorage[COMMON][id].Port + "&fav=0";
                    PostData("setStation", tosend);
                }
            }
        }
        clearPlayList(storage);
        localStorage.setItem("RadiolaStorage", JSON.stringify(RadiolaStorage));
    } else promptworking("");
    refreshList();
}

function clearPlayList(PlayList) {
    RadiolaStorage[PlayList].splice(0, RadiolaStorage[PlayList].length);
    RadiolaStorage[PlayList + "_select"] = 0;
    if (PlayList == COMMON)
        current_total = 0;
}

function ircode(mode) {
    try {
        PostData("ircode", "mode=" + mode + "&");
    } catch (e) { console.log("err: " + e); }
}

function upgrade() {
    try {
        PostData("upgrade");
    } catch (e) { console.log("err: " + e); }

    //	alert("Rebooting to the new release\nPlease refresh the page in few seconds.");
}
function getversion() {
    var arr = JSON.parse(PostData("version"));
    document.getElementById('currentrelease').innerHTML = arr["RELEASE"] + " Rev: " + arr["REVISION"];
    if (arr["curtemp"] != "000.0") {
        document.getElementById('curtemp').innerHTML = parseFloat(arr["curtemp"]) + ' ℃';
        document.getElementById('_temp').style.display = "block";
    }
    if (arr["rpmfan"] != "0000") {
        document.getElementById('rpmfan').innerHTML = arr["rpmfan"] + ' Об/м';
        document.getElementById('_fan').style.display = "block";
    }
    PostData("version");
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
    } catch (e) { console.log("err: " + e); }
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
    } catch (e) { console.log("err: " + e); }
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
        if (document.getElementById('newrelease').innerHTML != document.getElementById('currentrelease').innerHTML) {
            document.getElementById('upd').style.backgroundColor = "red";
        }
    }
    xhr.open("GET", "https://serverdoma.ru/esp32/version32.php", false);
    try {
        xhr.send(null);
    } catch (e) { console.log("err: " + e); }
    checkinfos();
    //checkhistory();
}

/// refresh the stations list by reading a file
function downloadStations() {
    current_total = RadiolaStorage[COMMON].length;
    var count = 0;
    if (window.File && window.FileReader && window.FileList && window.Blob) {
        var reader = new FileReader();
        promptworking(working);

        reader.onload = function () {

            var lines, errTxt = '\n', errName = 0, errUrl = 0, errPath = 0,
                errPort = 0, errAll = false, txt;

            lines = this.result.split(/\r?\n/);
            if (lines[0].includes("#EXTM3U")) {
                var getURL, nameSt, port;

                for (var line = 1; line < lines.length; line++) {
                    //
                    if (lines[line].includes("#EXTINF")) {
                        nameSt = lines[line].slice(lines[line].indexOf(',') + 1).trim();
                        if (nameSt.length > 64) {
                            nameSt = nameSt.substring(64);
                            errName++;
                            errAll = true;
                        }
                    } else {

                        try {
                            getURL = new URL(lines[line]);
                            port = getURL.port;
                            if (getURL.port == "") {
                                port = "80";
                            } else if (parseInt(getURL.port) > 65535) {
                                errPort++;
                                errAll = true;
                                count--;
                                continue;
                            } else if (getURL.hostname.length > 73) {
                                errUrl++;
                                errAll = true;
                                count--;
                                continue;
                            } else if (getURL.pathname.length > 116) {
                                errPath++;
                                errAll = true;
                                count--;
                                continue;
                            }

                            RadiolaStorage[COMMON].push(
                                {
                                    "Name": nameSt,
                                    "URL": getURL.hostname,
                                    "File": getURL.pathname,
                                    "Port": port,
                                    "FAV": false
                                });
                            var tosend = "&ID=" + count + "&name=" + nameSt + "&url=" + getURL.hostname +
                                "&file=" + getURL.pathname + "&port=" + port + "&fav=0";
                            PostData("setStation", tosend);
                            count++
                            if (count == maxStation) {
                                errTxt = "\nДостигнут порог ограничения количества радиостанций: " + count + " !";
                                errAll = true;
                                break;
                            }
                        } catch (e) {
                            console.log("err: " + e + "\nCounter: " + count + "\nВсего строк: " + lines.length);
                            continue;
                        }
                    }
                }
                current_total = count;
                PostData("setStation", "&save=1");
                localStorage.setItem("RadiolaStorage", JSON.stringify(RadiolaStorage));
                txt = "Корретных станций загружено: " + count;
                if (errAll) {
                    alert("Плейлист содержит ошибки!!\n" + errTxt +
                        "\nОбрезано названий станций: " + errName +
                        "\nПревышена допустимая длина\n" +
                        "\nДомен:" + errUrl +
                        "\nURL: " + errPath +
                        "\n\n" + txt);
                }
                refreshList();
                promptworking(txt);
            } else {
                txt = "Пожалуйста, выберите файл с данными M3U.";
                alert(txt);
                promptworking(txt)
                return;
            }
        };
        var file = document.getElementById('file_common').files[0];
        if (file == null) {
            alert("Пожалуйста, выберите файл.");
        } else {
            if ((current_total != 0) && confirm("Текущий плейлист не пуст! Вы хотите добавить новый плейлист в список?\n" +
                "Если нет, список будет перезаписан.")) {
                count = current_total;
            } else {
                if (current_total != 0) {
                    clearPlayList(COMMON);
                    localStorage.setItem("RadiolaStorage", JSON.stringify(RadiolaStorage));
                    PostData("clear");
                }

            }
            promptworking(working);
            reader.readAsText(file);
        }
    }
}

function dragStart(ev) {
    tblEvn = ev.target;
}

function setPLS(el) {
    try {
        if (el === COMMON || el === 0) PlayList = COMMON
        else if (el === FAVORITES || el === 1) PlayList = FAVORITES
        else {
            PlayList = el.parentNode.getAttribute('id').replace("tbody_", '');
        }
        current_total = RadiolaStorage[PlayList].length;
    } catch (e) {
        console.log("setPLS err: " + e, el);
    }
}

function dragEnd() {
    setPLS(tblEvn);
    var tbl = tblEvn.parentNode;
    for (var i = 0; i < tbl.rows.length; i++) {
        var parser = document.createElement('a');
        var name = tbl.rows[i].cells[1].innerText;
        var furl = tbl.rows[i].cells[2].innerText;
        parser.href = "http://" + furl;
        var url = parser.hostname;
        var file = parser.pathname + parser.hash + parser.search;
        var port = parser.port;
        if (!port) port = 80;

        RadiolaStorage[PlayList][i].Name = name;
        RadiolaStorage[PlayList][i].URL = url;
        RadiolaStorage[PlayList][i].File = file;
        RadiolaStorage[PlayList][i].Port = port;

        tbl.rows[i].cells[0].innerText = (i + 1).toString();
        if (PlayList == COMMON) {
            // RadiolaStorage[PlayList][i].ID = tbl.rows[i].cells[0].id.split('_')[1];
            if (document.getElementById("fav_" + i).className == 'icon-heart-o')
                RadiolaStorage[PlayList][i].FAV = false;
            else
                RadiolaStorage[PlayList][i].FAV = true;
        }
        tbl.rows[i].cells[0].id = 'num_' + i;
    }
    if (PlayList == COMMON) {
        stchanged = true;
        document.getElementById("stsave").disabled = false;
    } else {
        localStorage.setItem("RadiolaStorage", JSON.stringify(RadiolaStorage));
    }
}

function dragOver(e) {
    let children = Array.from(e.target.parentNode.parentNode.children);
    if (children.indexOf(e.target.parentNode) > children.indexOf(tblEvn))
        e.target.parentNode.after(tblEvn);
    else
        e.target.parentNode.before(tblEvn);
}

function stChanged() {
    var i, tosend;

    function fillInfo(ind) {
        var name = RadiolaStorage[COMMON][ind].Name;
        var url = RadiolaStorage[COMMON][ind].URL;
        var file = RadiolaStorage[COMMON][ind].File;
        var port = RadiolaStorage[COMMON][ind].Port;
        var fav = 0;
        if (RadiolaStorage[COMMON][ind].FAV) {
            fav = 1;
            RadiolaStorage[FAVORITES].push({
                "ID": ind,
                "Name": name,
                "URL": url,
                "File": file,
                "Port": port
            });
        }
        tosend = "&ID=" + ind + "&name=" + name + "&url=" + url +
            "&file=" + file + "&port=" + port + "&fav=" + fav;
        PostData("setStation", tosend);
    }
    if (stchanged && confirm("Список изменен. Вы хотите сохранить измененный список?")) {
        localStorage.setItem("RadiolaStorage", JSON.stringify(RadiolaStorage));
        promptworking(working);
        var arr = JSON.parse(PostData("getStation", "&ID=0"));
        if (parseInt(arr["Total"], 10) > RadiolaStorage[COMMON].length) {
            PostData("clear");
        }
        if (RadiolaStorage[FAVORITES].length)
            clearPlayList(FAVORITES);
        for (i = 0; i < (RadiolaStorage[COMMON].length); i++) {
            try {
                fillInfo(i);
            } catch (e) { console.log("err: " + e); }
        }
        PostData("setStation", "&save=1");
    } else
        RadiolaStorage = JSON.parse(localStorage.getItem("RadiolaStorage"));
    stchanged = false;
    document.getElementById("stsave").disabled = true;
    refreshList();
    promptworking("");
}

function checkFavorit(el) {
    var pos = el.getElementsByTagName("i")[0].className;
    var ID = el.parentNode.children[0].id.split('_')[1];
    var fav = false;
    if (pos == 'icon-heart-o') {
        el.getElementsByTagName("i")[0].className = 'icon-heart';
        fav = true;
    }
    else {
        el.getElementsByTagName("i")[0].className = 'icon-heart-o';
    }
    RadiolaStorage[COMMON][ID].FAV = fav;
    stchanged = true;
    document.getElementById("stsave").disabled = false;
}



function loadStations(PlayList) {
    var arr;
    var new_tbody = document.createElement('tbody'),
        idlist, idstr, select,
        idx;
    current_total = RadiolaStorage[PlayList].length;
    if (!current_total) {
        document.getElementById('download_common').value = "Загрузить плейлист";
        // document.getElementById("table_" + PlayList).caption.innerHTML = "<h1>Список станций пуст</h1><br>";
        document.getElementById("head_" + PlayList).style.display = 'none';
        document.getElementById("select_" + PlayList).style.display = 'none';

    } else {
        document.getElementById('download_common').value = "Добавить плейлист";
        document.getElementById("drag_common").innerHTML = "Вы можете перетаскивать любую строку, чтобы отсортировать список.";
        document.getElementById("head_" + PlayList).style.display = '';
        document.getElementById("select_" + PlayList).style.display = '';

        function cploadStations(idx, arr) {
            var tr = document.createElement('TR'),
                td = document.createElement('TD');
            if (PlayList == COMMON) {
                tr.draggable = "true";
                tr.ondragstart = dragStart;
                tr.ondragover = dragOver;
                tr.ondragend = dragEnd;
            }
            // ID
            td.appendChild(document.createTextNode(idx + 1));
            td.style.textAlign = "center";
            td.style.verticalAlign = "middle";
            if (PlayList == COMMON)
                td.style.cursor = "move";
            td.id = 'num_' + idx;
            tr.appendChild(td);
            // Name
            td = document.createElement('TD');
            td.style.whiteSpace = "nowrap";
            td.style.overflow = "hidden";
            td.style.textOverflow = "ellipsis";
            td.setAttribute('onclick', 'playEditStation(this.parentNode);');
            td.innerHTML = arr["Name"];
            tr.appendChild(td);
            if (PlayList == COMMON) {
                // URL
                td = document.createElement('TD');
                td.style.display = 'none';
                td.appendChild(document.createTextNode(arr["URL"] + ":" + arr["Port"] + arr["File"]));
                tr.appendChild(td);
                // FAV     
                td = document.createElement('TD');
                var fav = document.createElement('i');
                var title = "Добавить в ИЗБРАННОЕ";
                if (!arr["FAV"]) {
                    fav.className = 'icon-heart-o';
                }
                else {
                    fav.className = 'icon-heart';
                    title = "Удалить из ИЗБРАННОГО";
                }
                fav.setAttribute('onclick', 'checkFavorit(this.parentNode)');
                fav.setAttribute('title', title);
                fav.id = 'fav_' + idx;
                td.appendChild(fav);
                td.className = 'lst';
                tr.appendChild(td);

                // editStation button
                td = document.createElement('TD');
                td.className = 'lst';
                var btn = document.createElement('i');
                btn.setAttribute('onclick', 'editStation(this.parentNode)');
                btn.setAttribute('title', 'Редактировать станцию');
                btn.className = 'icon-page-edit';
                td.appendChild(btn);
            } else {
                td = document.createElement('TD');
                td.className = 'lst';
            }
            // removeStation button
            btn = document.createElement('i');
            btn.setAttribute('onclick', 'removeStation(this.parentNode)');
            btn.setAttribute('title', 'Удалить станцию из списка');
            btn.className = 'icon-trash-bin';
            td.appendChild(btn);

            tr.appendChild(td);
            if (idlist === idstr) {
                setEditBackground(tr);
                editIndex = tr;
                editPlaying = true;
            }
            new_tbody.appendChild(tr);
        }
        select = document.getElementById('select_' + PlayList);
        try {
            idlist = select.options[select.selectedIndex].value;
        } catch (e) { idlist = 0; }
        for (idx = 0; idx < current_total; idx++) {
            idstr = idx.toString();

            try {
                arr = RadiolaStorage[PlayList][idx];
                cploadStations(idx, arr);
            } catch (e) { console.log("err: " + e); }

        }
    }

    new_tbody.id = "tbody_" + PlayList;
    var old_tbody = document.getElementById("table_" + PlayList).getElementsByTagName('tbody')[0];
    old_tbody.parentNode.replaceChild(new_tbody, old_tbody);
}
//Load the selection with all stations
function loadStationsList(PlayList) {
    var foundNull = false,
        max = RadiolaStorage[PlayList].length,
        id, arr, select, i;
    select = document.getElementById("select_" + PlayList);
    for (i = select.options.length - 1; i >= 0; i--) {
        select.remove(i);
    }

    function cploadStationsList(id, arr) {
        foundNull = false;
        if (arr["Name"].length > 0) {
            var opt = document.createElement('option');
            opt.appendChild(document.createTextNode((id + 1) + ": " + arr["Name"]));
            opt.setAttribute("value", id);
            select.add(opt);
        } else foundNull = true;
        return foundNull;
    }
    select.disabled = true;
    promptworking(working);
    for (id = 0; id < max; id++) {
        var idstr = id.toString();

        try {
            arr = RadiolaStorage[PlayList][idstr];
            foundNull = cploadStationsList(id, arr);
        } catch (e) { console.log("err: " + e); }
    }

    promptworking("");
    select.disabled = false;
    select.options.selectedIndex = RadiolaStorage[PlayList + "_select"];
}

function printList() {
    var html = "<html>";
    var id, arr;
    html += "</html>";
    html += "<h1>Список станций Радиолы</h1><br/><hr><br/>";
    for (id = 0; id < current_total; id++) {
        var idstr = id.toString();
        if (localStorage.getItem(idstr) != null) {
            try {
                arr = JSON.parse(localStorage.getItem(idstr));
            } catch (e) { console.log("err: " + e); }
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

//ScrollUp
function ScrollUp() {
    var t, s;
    s = document.body.scrollTop || window.pageYOffset;
    t = setInterval(function () { if (s > 0) window.scroll(0, s -= 5); else clearInterval(t) }, 5);
}
//
function PostData(Address, Data) {
    if (typeof (Data) == 'undefined') Data = '';
    let xhr = new XMLHttpRequest();

    // var ctype = "application/x-www-form-urlencoded",
    // cjson = "application/json";

    xhr.open("POST", Address, false);
    xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    try {
        xhr.send(Data);
        if (xhr.readyState != 4 && xhr.status != 200) {
            console.log("PostData error: xhr.status: " + xhr.status + ' xhr.statusText: ' + xhr.statusText);
            return null;
        } else {
            // console.log("xhr.responseText: " + xhr.responseText);
            return xhr.responseText;
        }
    } catch (e) {
        console.log("PostData err: " + e);
    }
}

function gotoTAB(mTab, subTab) {
    /* Карта вкладок
        tabbis.onClick(0,0) - PLAYER
            tabbis.onClick(1,0) - SELECT LIST COMMON
            tabbis.onClick(1,1) - SELECT LIST FAVORITES
        
        tabbis.onClick(0,1) - STATIONS
            tabbis.onClick(2,0) - PLAYLIST COMMON
            tabbis.onClick(2,1) - PLAYLIST FAVORITES
        
        tabbis.onClick(0,2) - SOUND
            tabbis.onClick(3,0) - COMPUTER
            tabbis.onClick(3,1) - VS1053
            tabbis.onClick(3,2) - BT201
        
        tabbis.onClick(0,3) - SETTINGS
            tabbis.onClick(4,0) - WIFI
            tabbis.onClick(4,1) - GPIOS
            tabbis.onClick(4,2) - REMOTE
            tabbis.onClick(4,3) - OPTIONS
            tabbis.onClick(4,4) - UPDATE
    */
    switch (mTab) {
        case "WIFI":
            tabbis.onClick(0, 3);
            tabbis.onClick(4, 0);
            break;
        case "PLAYER":
            tabbis.onClick(0, 0);
            break;
        case "SOUND":
            if (typeof (subTab) == 'undefined') {
                break;
            } else {
                tabbis.onClick(0, 2);
                if (subTab === "VS1053") {
                    tabbis.onClick(3, 1);
                }
            }
            break;
        case "AUX":
            tabbis.onClick(3, 0);
            break;
        case "VS1053":
            tabbis.onClick(3, 1);
            break;
        case "BT201":
            tabbis.onClick(3, 2);
            break;
    }

}
function getStations() {
    var id, fav;
    var arr = JSON.parse(PostData("getStation", "&ID=0"));
    current_total = parseInt(arr["Total"], 10);
    if (current_total == 0) {
        return;
    } else {
        promptworking(working);
        for (id = 0; id < current_total; id++) {
            fav = false;
            arr = JSON.parse(PostData("getStation", "&ID=" + id));

            if (arr["fav"] == "1") {
                fav = true;
                RadiolaStorage[FAVORITES].push({
                    "ID": id,
                    "Name": arr["Name"],
                    "URL": arr["URL"],
                    "File": arr["File"],
                    "Port": arr["Port"]
                });
            }
            RadiolaStorage[COMMON].push({
                "Name": arr["Name"],
                "URL": arr["URL"],
                "File": arr["File"],
                "Port": arr["Port"],
                "FAV": fav
            });
        }
        localStorage.setItem("RadiolaStorage", JSON.stringify(RadiolaStorage));
        promptworking("");
    }
}
document.addEventListener("DOMContentLoaded", function () {

    checkwebsocket();

    if (localStorage.getItem('RadiolaStorage') !== null) {
        localStorage.removeItem("RadiolaStorage");

    }

    try {
        getStations();
    } catch (e) { console.log("err: " + e); }

    try {
        if (localStorage.getItem('tabbis') !== null) {
            localStorage.removeItem("tabbis");
        }
        tabbis.init();
    } catch (e) {
        console.log("err: " + e);
    }
    refreshList();
    document.getElementById("PLAYER").addEventListener("click", function () {
        if (stchanged) stChanged();
        refresh();
        refreshList();
        curtab = "PLAYER";
    });
    document.getElementById("STATIONS").addEventListener("click", function () {
        if (stchanged) stChanged();
        refreshList();
        curtab = "STATIONS";
    });

    document.getElementById("pls_common").addEventListener("click", function () {
        if (stchanged) stChanged();
        refreshList();
        curtab = "pls_common";
    });
    document.getElementById("pls_favorites").addEventListener("click", function () {
        if (stchanged) stChanged();
        refreshList();
        curtab = "pls_favorites";
    });
    document.getElementById("list_common").addEventListener("click", function () {
        if (stchanged) stChanged();
        refreshList();
        curtab = "list_common";
    });
    document.getElementById("list_favorites").addEventListener("click", function () {
        if (stchanged) stChanged();
        refreshList();
        curtab = "list_favorites";
    });


    document.getElementById("SOUND").addEventListener("click", function () {
        if (stchanged) stChanged();
        curtab = "SOUND";
        hardware(0);
    });

    document.getElementById("AUX").addEventListener("click", function () {
        curtab = "AUX";
        audioinput = AUX;
        hardware(1);
    });
    document.getElementById("VS1053").addEventListener("click", function () {
        curtab = "VS1053";
        audioinput = VS1053;
        hardware(1);
    });
    document.getElementById("BT201").addEventListener("click", function () {
        curtab = "BT201";
        audioinput = BT201;
        hardware(1);
    });

    document.getElementById("SETTINGS").addEventListener("click", function () {
        if (stchanged) stChanged();
        curtab = "SETTINGS";
    });
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

    window.addEventListener("keydown", function (event) {
        if (event.defaultPrevented) {
            return;
        }
        if (event.ctrlKey) {
            switch (event.key) {
                case ' ':
                    if (editPlaying) stopStation();
                    else playStation(PlayList);
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
            refreshList();
            editPlaying = ed;
        }
    }, true);

    if (intervalid != 0) window.clearTimeout(intervalid);
    intervalid = 0;
    if (timeid != 0) window.clearInterval(timeid);
    timeid = window.setInterval(dtime, 1000);
    if (window.location.hostname == "192.168.4.1") {
        document.getElementById("WIFI").click();
        gotoTAB("WIFI");
    }
    wifi(0);
    hardware(0);
    autostart();
    checkversion();
    var day = new Date();
    let days_f = ['воскресенье', 'понедельник', 'вторник', 'среда', 'четверг', 'пятница', 'суббота'];
    let days = ['ВС', 'ПН', 'ВТ', 'СР', 'ЧТ', 'ПТ', 'СБ'];
    let months = ['января', 'февраля', 'марта', 'апреля', 'мая', 'июня', 'июля', 'августа', 'сентября', 'октября', 'ноября', 'декабря'];
    document.getElementById('q1').innerHTML = day.getDate() + ' ' + months[day.getMonth()] + ', ' + days[day.getDay()] + '. ' + day.getFullYear() + ' г.';
    document.getElementById('q2').innerHTML = day.getDate() + ' ' + months[day.getMonth()] + ', ' + days_f[day.getDay()];
    refresh();
});
