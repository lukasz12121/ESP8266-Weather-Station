const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <script src="https://code.highcharts.com/highcharts.js"></script>
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
table {
  font-family: arial, sans-serif;
  border-collapse: collapse;
  width: 100%;
}

td, th {
  border: 1px solid #dddddd;
  text-align: left;
  padding: 8px;
}

tr:nth-child(even) {
  background-color: #dddddd;
} 
.switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}

.button {
  background-color: #808080;
  border: none;
  color: white;
  padding: 16px 32px;
  text-align: center;
  font-size: 16px;
  margin: 4px 2px;
  opacity: 0.6;
  transition: 0.3s;
  display: inline-block;
  text-decoration: none;
  cursor: pointer;
}

.button:hover {opacity: 1}
</style>
</head>
<body>
  <h2>ESP8266 HTTP Server</h2>
  <button class="button" onclick="logoutButton()">Logout</button>
  <p>Light switch - <span id="state">%STATE%</span></p>
  %BUTTONPLACEHOLDER%
  <h3>Current values(refresh every 10 sec)</h3>
  <p>
  <table>
  <tr>
    <th><strong>Reading type</strong></th>
    <th><strong>Reading value</strong></th>
    <th><strong>Units</strong></th>
    <th><strong>Sensor</strong></th>
  </tr>
  <tr>
    <th>Altitude</th>
    <th id="altitude">%ALTITUDE%</th>
    <th>m</th>
    <th>BMP280</th>
  </tr>
    <tr>
    <th>Pressure</th>
    <th id="pressure">%PRESSURE%</th>
    <th>hPa</th>
    <th>BMP280</th>
  </tr>
    <tr>
    <th>Gas/smoke level</th>
    <th id="gas_level">%GAS_LEVEL%</th>
    <th>ppm</th>
    <th>MQ-2</th>
  </tr>
  <tr>
    <th>Temperature</th>
    <th id="temperature">%TEMPERATURE%</th>
    <th>C</th>
    <th>DHT11</th>
  </tr>
  <tr>
    <th>Humidity</th>
    <th id="humidity">%HUMIDITY%</th>
    <th>%</th>
    <th>DHT11</th>
  </tr>
</table>
</p>
<div id="chart-altitude" class="container"></div>
<br>
<div id="chart-pressure" class="container"></div>
<br>
<div id="chart-gas_level" class="container"></div>
</body>
<div id="chart-temperature" class="container"></div>
<br>
<div id="chart-humidity" class="container"></div>
<br>
<script>
var chartAltitude = new Highcharts.Chart({
  chart:{ renderTo:'chart-altitude' },
  title: { text: 'BMP280 Altitude' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#01F6FF' }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Altitude (m)' }
  },
  credits: { enabled: false }
});

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("altitude").innerHTML = this.responseText;
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      if(chartAltitude.series[0].data.length > 40) {
        chartAltitude.series[0].addPoint([x, y], true, true, true);
      } else {
        chartAltitude.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/altitude", true);
  xhttp.send();
}, 10000 ) ;

var chartPressure = new Highcharts.Chart({
  chart:{ renderTo:'chart-pressure' },
  title: { text: 'BMP280 Pressure' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#FF0000' }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Pressure (hPa)' }
  },
  credits: { enabled: false }
});

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("pressure").innerHTML = this.responseText;
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      if(chartPressure.series[0].data.length > 40) {
        chartPressure.series[0].addPoint([x, y], true, true, true);
      } else {
        chartPressure.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/pressure", true);
  xhttp.send();
}, 10000 ) ;

var chartGas = new Highcharts.Chart({
  chart:{ renderTo:'chart-gas_level' },
  title: { text: 'MQ-2 Gas/smoke level' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#CCFF01' }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Gas/smoke level (ppm)' }
  },
  credits: { enabled: false }
});

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("gas_level").innerHTML = this.responseText;
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      if(chartGas.series[0].data.length > 40) {
        chartGas.series[0].addPoint([x, y], true, true, true);
      } else {
        chartGas.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/gas_level", true);
  xhttp.send();
}, 10000 ) ;

var chartTemperature = new Highcharts.Chart({
  chart:{ renderTo:'chart-temperature' },
  title: { text: 'DHT11 Temperature' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#059e8a' }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Temperature (Celsius)' }
  },
  credits: { enabled: false }
});

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      if(chartTemperature.series[0].data.length > 40) {
        chartTemperature.series[0].addPoint([x, y], true, true, true);
      } else {
        chartTemperature.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

var chartHumidity = new Highcharts.Chart({
  chart:{ renderTo:'chart-humidity' },
  title: { text: 'DHT11 Humidity' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#18009c' }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Humidity (percent)' }
  },
  credits: { enabled: false }
});

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartHumidity.series[0].data.length > 40) {
        chartHumidity.series[0].addPoint([x, y], true, true, true);
      } else {
        chartHumidity.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ 
    xhr.open("GET", "/update?state=1", true); 
    document.getElementById("state").innerHTML = "ON";  
  }
  else { 
    xhr.open("GET", "/update?state=0", true); 
    document.getElementById("state").innerHTML = "OFF";      
  }
  xhr.send();
}
function logoutButton() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/logout", true);
  xhr.send();
  setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
} 
</script>
</html>)rawliteral";
