<?php
/*
*
*Install : php5-cli.deb on debian 
*
*/

/*
* ?get=ping
* Ping all devices inside log table and dump result in network table.
* The result is printed with JSON representation
*
* ?get=ping&&address=IPV6address
* Ping a specific device and print the result
*/
$get		= isset($_GET["get"])?$_GET["get"]:"";
$ressource 	= isset($_GET["ressource"])?$_GET["ressource"]:"*";
$address	= isset($_GET["address"])?$_GET["address"]:"*";

for($i=1; $i < $_SERVER["argc"]; $i++){

	if(strstr($_SERVER["argv"][$i],"ressource=")){
		$tmp = explode("=",$_SERVER["argv"][$i]);
		$ressource = $tmp[1];
	}
	if(strstr($_SERVER["argv"][$i],"address=")){
		$tmp = explode("=",$_SERVER["argv"][$i]);
		$address = $tmp[1];
	}
	if(strstr($_SERVER["argv"][$i],"get=")){
		$tmp = explode("=",$_SERVER["argv"][$i]);
		$get = $tmp[1];
	}
}


function Emma_connection_log(){
	mysql_query("INSERT INTO log (address, ressource, unit, time, value) 
			VALUES ('".$_SERVER['REMOTE_ADDR']."',
				'".$_SERVER['HTTP_USER_AGENT']."',
				'int',
				".(time()*1000 + 7200*1000).",
				1)");
}

function Emma_connection(){
$serv = mysql_connect("localhost","root","root");
mysql_select_db("Emma-site",$serv);
}

function ping($address){
if($address == "")
	return false;

$result = shell_exec  ("ping6 -c 1 ".$address);
if(strstr($result,"100% packet loss") != "" || $result == ""){
	echo "	{\n
		\"address\":\"".$address."\",\n
		\"status\":\"Not connected\"\n
		}";
	return false;
	}
else 	{
	echo "	{\n
		\"address\":\"".$address."\",\n
		\"status\":\"Connected\"\n
		}";
	return true;
	}
}

switch($get){
	case "notify":
		$result = file_get_contents("http://[".$_SERVER['REMOTE_ADDR']."]/data/*/");
		if($result == false);
		$Datas  = json_decode($result);
		
		Emma_connection();
		for ($i=0; $i < count($Datas); $i++){
			if(!mysql_query("INSERT INTO log (address, ressource, unit, time, value) 
					VALUES ('".$_SERVER['REMOTE_ADDR']."',
						'".($Datas[$i]->name)."',
						'".($Datas[$i]->unit)."',
						".(time()*1000  + 7200*1000).",
						".($Datas[$i]->InstatenousRate).")"));
		}
	break;

	case "log":
		Emma_connection();
		$sql = "SELECT * FROM log ";
		
		if($ressource != "*" || $address != "*")
			$sql .= "WHERE ";
		
		if($ressource 	!= "*" && $ressource 	!= "")	$sql .= "ressource='$ressource'";
		if($ressource 	!= "*" && $ressource 	!= "" && $address 	!= "*" && $address 	!= "")
			$sql .= " AND ";
		if($address 	!= "*" && $address 	!= "") $sql .= "address='$address'";
		
		$sql .= " ORDER BY address, ressource, time";
		
		$sql = mysql_query($sql);
		
		// iterate over every row
		while ($row = mysql_fetch_assoc($sql)) {
			// for every field in the result..
			for ($i=0; $i < mysql_num_fields($res); $i++) {
				$info = mysql_fetch_field($res, $i);
				$type = $info->type;
		
			// cast for real
			if ($type == 'real')
				$row[$info->name] = doubleval($row[$info->name]);
			// cast for int
			if ($type == 'int')
				$row[$info->name] = intval($row[$info->name]);
			}
			$rows[] = $row;
		}
		
		// JSON-ify all rows together as one big array
		$json = json_encode($rows);
		
		if($json != "null") echo $json;
		else echo "[]";
	break;

	case "network":
		Emma_connection();
		$sql= mysql_query("SELECT * FROM network");
		echo "[\n";
		$first = true;
		while($ligne = mysql_fetch_array($sql)){
			if(!$first)
				echo ",";
			echo "{\n\"address\":\"".$ligne["address"]."\",\n\"type\":\"".$ligne["type"]."\",\n\"state\":\"".$ligne["state"]."\",\n\"groupe\":\"".$ligne["groupe"]."\"\n}";
			}
		echo "]\n";
	break;

	case "ping" : 
		if($address != "*"){
			ping($address);
			exit(0);
		}
		Emma_connection();
		$sql = mysql_query("SELECT * FROM log ORDER BY address");
		$precedent = "";
		while($ligne = mysql_fetch_array($sql)){

			if($ligne["address"] != $precedent){
				$precedent = $ligne["address"];

				if(ping($ligne["address"])){
					$tmp = mysql_query("SELECT * FROM network WHERE address='".$ligne["address"]."'");	
					$tmp = mysql_fetch_array($tmp);
					if($tmp) mysql_query("UPDATE network SET state='Connected' WHERE id='$tmp[address]'");

					else mysql_query("INSERT INTO network (address, type, state, groupe)
							VALUES('".$ligne["address"]."','','Connected','')");
					}
				else {
					mysql_query("DELETE FROM network WHERE address='".$ligne["address"]."'");
				}

			}
		}
		break;

	default :;
}

?>