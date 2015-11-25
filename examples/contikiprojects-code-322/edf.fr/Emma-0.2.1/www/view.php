
<?php
require_once "proxy.php";

Emma_connection();
Emma_connection_log();


?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
	<head>
		<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
		<title>E.M.M.A Viewer</title>
		
		
		<!-- 1. Add these JavaScript inclusions in the head of your page -->
		<script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jquery/1.3.2/jquery.min.js"></script>
		<script type="text/javascript" src="Highcharts-1.2.5/js/highcharts.js"></script>
		<script type="text/javascript" src="JSON/jquery.json-2.2.min.js"></script>

		<style text="text/css">
		body{
		font-family : Times New Roman, Times, serif;
		}

		#LabelMenu{
		position:absolute;
		top:20px;
		left:10px;
		font-weight:bold;
		color:green;
		font-size:20px;

		}

		.ObjectIPsSelectorLabel{
		position:absolute;
		top:60px;
		left:10px;
		font-weight:bold;
		}

		.ObjectIPsSelector{
		position:absolute;
		top:100px;
		left:10px;
		border-style:solid;
		border-color:black;
		border-width:1px;
		min-width:200px;
		width:200px;
		}

		#RessourceSelectorLabel{
		position:absolute;
		top:60px;
		right:10px;
		font-weight:bold;
		background-color:#A2BCFF;
		width:200px;
		color:white;
		}

		#TAGressource {
		position:absolute;
		top:100px;
		right:10px;
		border-style:solid;
		border-color:black;
		border-width:1px;
		min-width:200px;
		width:200px;
		color:#A2BCFF;
		}

		a{
		color:black;
		text-decoration:none;
		}
		a:hover{
		color:black;
		font-weight:bold;
		}
		</style>
		<!--[if IE]>
			<script type="text/javascript" src="../js/excanvas.compiled.js"></script>
		<![endif]-->
		
		
		<!-- 2. Add the JavaScript to initialize the chart on document ready -->
<script type="text/javascript">
var uri_proxy = 'proxy.php';
var chart;
var ressource = '*';
var address = '';
var ProcessLoaded = false;


Array.prototype.contains = function (element) {
for (var i = 0; i < this.length; i++) {
if (this[i] == element) {
return true;
}
}
return false;
}

var ListObjectIPs = new Array();
function doGETListObjectIPs(){
var ajaxOpts = {  
	type: 'get',
	url: uri_proxy,	
	data: {get:'log'},
	success: 
		function(data) {  
		//	ListObjectIPs = new Array();

			datas = $.evalJSON(data);
			$.each(datas, function(i,item){
				var isContains = false;
				for(var k=0; k<ListObjectIPs.length; k++){
					if(ListObjectIPs[k].address == item.address)
						isContains = true;
				}
				if(!isContains)
					ListObjectIPs.push({address:item.address,status:false});
			});

			// Get network list of connected device
			var result = '';
			var ajaxOpts2 = {  
				type: 'get',
				url: uri_proxy,
				data : {get:'network'},
				success: function(data) {  
					result = $.evalJSON(data);
					$.each(result, function(i,item){
						
						var isContains = false;
						for(var k=0; k<ListObjectIPs.length; k++){
							if(ListObjectIPs[k].address == item.address)
								isContains = true;
						}
						if(!isContains)
							ListObjectIPs.push({address:item.address,status:false});

						for(var k=0;k<ListObjectIPs.length;k++)
							if(ListObjectIPs[k].address == item.address)
								if(item.state == 'Connected')
									ListObjectIPs[k].status = true;
						});

					}
				};  
			$.ajax(ajaxOpts2); 
	
			result = '';
			for(j=0; j<ListObjectIPs.length;j++){
				result += "<a href=\"#\" style=\"color:"+(ListObjectIPs[j].status?"green":"red")+"\" OnClick=\"address='" + ListObjectIPs[j].address + "'; chart.destroy(); CreateChart();\">"+ ListObjectIPs[j].address + "</a><br>";
				}
			document.getElementById('TAGObjectIPs').innerHTML = result;
		}

	};  
$.ajax(ajaxOpts);  
}

function doGETListRessource(ObjectIPAdd){
var ajaxOpts = {  
	type: 'get',
	url: uri_proxy,
	data : {get:'log',address:address},
	success: 
		function(data) {  
			var ListRessources = new Array();

			datas = $.evalJSON(data);
			$.each(datas, function(i,item){
				if(!ListRessources.contains(item.ressource))
					ListRessources.push(item.ressource);
			});

			// Synchronisation des listes locales et distantes
			var result = "* <a href=\"#\" OnClick=\"ressource='*'; chart.destroy(); CreateChart();\" style=\"color:#A2BCFF;\"> Toutes</a><br>";
			for(j=0; j<ListRessources.length;j++){
				result += "* <a href=\"#\" OnClick=\"ressource='" + ListRessources[j] + "'; chart.destroy(); CreateChart();\" style=\"color:#A2BCFF;\">"+ ListRessources[j] + "</a><br>";
				}
			document.getElementById('TAGressource').innerHTML = result;
		}

	};  
$.ajax(ajaxOpts);  
}

function FactoryChart() {

var series = chart.series;
//chart.showLoading();

var precedent = '';
var j = -1;
var ajaxOpts = {  
	type: 'get',
	url: uri_proxy,
	data: {get:'log',address:address, ressource: ressource},
	success: 
		function(data) {  
			chart.serie = new Array();
			datas = $.evalJSON(data);
			$.each(datas, function(i,item){

			
			if(precedent != item.ressource && item.ressource){
				if(!chart.get(item.ressource))
					chart.addSeries ({name:item.ressource,id:item.ressource,data:[]},false);

				series[++j].data = new Array();
				series[j].name = item.ressource;
				}
										
			series[j].addPoint([new Date().setTime(item.time) , item.value], false, false);
			precedent = item.ressource;
			});

		chart.redraw();
//		chart.hideLoading();  
		}

	};  
$.ajax(ajaxOpts);  
}

function CreateChart() {
	chart = new Highcharts.Chart({
	chart: {
		renderTo: 'container',
		defaultSeriesType: 'spline',
		margin: [50, 150, 60, 80],
		events: {
			load: function() {
				FactoryChart();
				if(!ProcessLoaded){
					setInterval(doGETListObjectIPs, 5000);
					setInterval(doGETListRessource, 2000);
					setInterval(FactoryChart, 4000);
					ProcessLoaded = true;
					}
				document.getElementById('RessourceSelectorLabel').innerHTML = 'Node : ['+address+']';
				}
			}
		},
	title: {
		text: 'EMMA Viewer',
		style: {
			margin: '10px 100px 0 0' // center it
		}
	},
	xAxis: {
		type: 'datetime',
		tickPixelInterval: 150
		},
	yAxis: {
		max: 255,
		min: 0,
		maxZoom: 0.1,
		title: {
			text: 'Value'
		},
		plotLines: [{
			value: 0,
			width: 1,
			color: '#808080'
		}]
	},
	tooltip: {
	formatter: function() {'auto'
			return '<b>'+ this.series.name +'</b><br/>Time: '+
				Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', this.x) +'<br/>'+ 
				Highcharts.numberFormat(this.y, 2);
		}
	},
	legend: {
		layout: 'vertical',
		style: {
			left: 'auto',
			bottom: 'auto',
			right: '10px',
			top: '100px'
		}
	},
	plotOptions: {
		series: {
			marker: {
				enabled: false,
				states: {
					hover: {
						enabled: true,
						radius: 3
						}
					}
				}
			}
	},
	series: []
	});
}

$(document).ready(CreateChart);

</script>
		
	<script type="text/javascript" src="http://www.highcharts.com/highslide/highslide-full.min.js"></script>
<script type="text/javascript" src="http://www.highcharts.com/highslide/highslide.config.js" charset="utf-8"></script>
<link rel="stylesheet" type="text/css" href="http://www.highcharts.com/highslide/highslide.css" />
</head>
	<body>
		<!-- 3. Add the container -->
		<div id="container" style="width: 800px; height: 400px; margin: 0 auto"></div>
		
		<p id="LabelMenu"> E.M.M.A Viewer v0.1</p>

		<p class="ObjectIPsSelectorLabel"> E.M.M.A's Network</p>
		<div id="TAGObjectIPs" class="ObjectIPsSelector"></div>

		<p id="RessourceSelectorLabel">
		<div id="TAGressource"></div>
</p>

	</body>
</html>
