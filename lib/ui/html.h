#ifndef _HTML_H_
#define _HTML_H_

const char indexTemplate[] = R"(

<!doctype html>
<html lang="en">
<head>
	<title>ESP32 AQI</title>
	<meta charset='utf-8'>
	<meta name='viewport' content='initial-scale=1, viewport-fit=cover'>
	<style>

.good {
  background-color: #68e143;
}
.moderate {
  background-color: #ffff55;
}
.unhealthy-for-sensitive-groups {
  background-color: #ef8533;
}
.unhealthy {
  background-color: #ea3324;
  color: #fff;
}
.very-unhealthy {
  background-color: #8c1a4b;
  color: #fff;
}
.hazardous {
  background-color: #8c1a4b;
  color: #fff;
}
.very-hazardous {
  background-color: #731425;
  color: #fff;
}
div.aqiBox {
	position: relative;
	height: 6em;
	min-width: 6em;
	padding: 0.5em;
	margin: 0.75em;
	font-weight: bold;
	border: solid;
	border-color: black;
}
div.aqiBoxTitle {
	position: absolute;
	top: 0.3em;
	left: 0.5em;
	font-size: 1em;
}
div.aqiBoxAqi {
	font-size: 3em;
	text-align: center;
	width: 100%%;
}
div.aqiBoxDetails {
	position: absolute;
	bottom: 0.5em;
	right: 0.5em;
	font-size: 1em;
}
.all-center {
	margin: 0;
	position: absolute;
	top: 50%%;
	left: 50%%;
	transform: translate(-50%%, -50%%);
}

div.bonusText {
	margin: 1em;
	font-weight: bold;
	font-size: 2em;
}
	</style>
</head>

<body style="font-family: monospace" class="%s">

<div style="display:flex; justify-content:center; align-items:center; padding:1em">
<div>

	<div style="margin:3em">
	<H1 style="justify-content: center; text-align:center; font-size: min(15em,25vw); font-weight: bold; margin: 0">%d</h1>
	<H2 style="justify-content: center; text-align:center; font-size: min(3em,5vw); margin: 0">%s</h2>
	</div>

	<div style="display:flex; flex-wrap: wrap; justify-content:center; align-items:center; padding:1em; white-space:nowrap">
		<div class="aqiBox %s">
			<div class="aqiBoxTitle">PM 1.0</div>
			<div class="aqiBoxAqi all-center">%d</div>
			<div class="aqiBoxDetails">%d µg/m<sup>3</sup></div>
		</div>

		<div class="aqiBox %s">
			<div class="aqiBoxTitle">PM 2.5</div>
			<div class="aqiBoxAqi all-center">%d</div>
			<div class="aqiBoxDetails">%d µg/m<sup>3</sup></div>
		</div>

		<div class="aqiBox %s">
			<div class="aqiBoxTitle">PM 10.0</div>
			<div class="aqiBoxAqi all-center">%d</div>
			<div class="aqiBoxDetails">%d µg/m<sup>3</sup></div>
		</div>
	</div>

	<div style="display:flex; flex-wrap: wrap; justify-content:center; align-items:center; padding:1em; white-space:nowrap">
	<div class="aqiBox %s">
		<div class="aqiBoxTitle">CO2</div>
		<div class="aqiBoxAqi all-center" style="font-size:2.5em">%d</div>
		<div class="aqiBoxDetails">ppm</sup></div>
	</div>
	</div>

	<div style="display:flex; flex-wrap: wrap; justify-content:center; align-items:center; padding:1em; white-space:nowrap">
	  <div class="bonusText">%.1f°C</div>
	  <div class="bonusText">%.1f°F</div>
	  <div class="bonusText">RH: %.0f%%</div>
	  <div class="bonusText">%.2f hPa</div>
	</div>

	<div style="text-align:left">
Uptime: %s<br />
PMSA003:   particles/dL: >=0.3µm: %d >=0.5µm: %d >=1.0µm: %d >=2.5µm: %d >=5.0µm: %d >=10.0µm: %d
<br />
MH-Z19C:    CO2:  %dppm  %d°C     %.1f°F
<br />
DS-020-20:  CO2: %dppm 
<br />
BME280:                   %.2f°C  %.2f°F  %.2f Pa  RH: %.2f%%
<br />
	</div>

</div>
</div>

</body>
</html>
)";

#endif  // _HTML_H_
