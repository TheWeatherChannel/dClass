vcl 4.0;

import dclass;
#import urlcode;

backend default {
	.host = "localhost";
	.port = "80";
}

sub vcl_init {
	# load devicemap
	dclass.init_dclass("/some/path/dClass/dtrees/devicemap.dtree");

	# load browser dtree
	dclass.init_dclass_p("/some/path/dClass/dtrees/browser.dtree",1);
}

sub vcl_recv {
	# ua override
	if(req.url ~ "[^\w]ua="){
		set req.http.user-agent = regsub(req.url,"^.*[^\w]ua=([^&]*).*","\1");
		#set req.http.user-agent = urlcode.decode(regsub(req.url,"^.*[^\w]ua=([^&]*).*","\1"));
	}

	# do the lookup
	set req.http.dclass_devicemap = dclass.classify(req.http.user-agent);
	#set req.http.dclass_devicemap = dclass.classify_p(req.http.user-agent,0);
	set req.http.dclass_browser = dclass.classify_p(req.http.user-agent,1);

	# set dclass_type
	set req.http.dclass_type = "unknown";
	if(dclass.get_field("is_tablet") == "true"){
		set req.http.dclass_type = "tablet";
	} else if(dclass.get_field("is_wireless_device") == "true" && dclass.get_field("inputDevices") == "touchscreen"){
		set req.http.dclass_type = "smartphone";
	} else if(dclass.get_field("is_wireless_device") == "true"){
		set req.http.dclass_type = "phone";
	} else if(dclass.get_field("is_crawler") == "true"){
		set req.http.dclass_type = "crawler";
	} else if(dclass.get_field("is_desktop") == "true"){
		set req.http.dclass_type = "desktop";
	}

	# /uadiag commands
	if(req.url ~ "^/uadiag.js"){
		return (synth(849,"uadiag.js"));
	} else if(req.url ~ "^/uadiag"){
		return (synth(848,"uadiag"));
	}
}

# add dclass_type to the hash
sub vcl_hash {
	hash_data(req.url);

	if (req.http.host) {
		hash_data(req.http.host);
	} else {
		hash_data(server.ip);
	}

	#use this if you are serving different device content on the same URL
	hash_data(req.http.dclass_type);

	return (lookup);
}

sub vcl_synth {
	# plain text ua diag
	if(resp.status == 848){
		set resp.status = 200;
		set resp.http.Content-Type = "text/html; charset=utf-8";
		synthetic(
{"<html><title>UADIAG</title>
<body>
<h1>UADIAG</h1>
<pre>
User-Agent:         "} + req.http.User-Agent + {"

dClass version:     "} + dclass.get_version() + {"

dClass type:        "} + req.http.dclass_type + {"

DeviceMap comment:  "} + dclass.get_comment() + {"
DeviceMap id:       "} + req.http.dclass_devicemap + {"
DeviceMap device:   "} + dclass.get_field("vendor") + {" "} + dclass.get_field("model") + {"
DeviceMap display:  "} + dclass.get_ifield("displayWidth") + {"x"} + dclass.get_ifield("displayHeight") + {"
DeviceMap input:    "} + dclass.get_field("inputDevices") + {"
DeviceMap js:       "} + dclass.get_field("ajax_support_javascript") + {"
DeviceMap wireless: "} + dclass.get_field("is_wireless_device") + {"
DeviceMap tablet:   "} + dclass.get_field("is_tablet") + {"
DeviceMap crawler:  "} + dclass.get_field("is_crawler") + {"
DeviceMap desktop:  "} + dclass.get_field("is_desktop") + {"

browser comment  :  "} + dclass.get_comment_p(1) + {"
browser id:         "} + req.http.dclass_browser + {"
browser name:       "} + dclass.get_field_p("browser",1) + {"
browser ver:        "} + dclass.get_ifield_p("version",1) + {"
browser os:         "} + dclass.get_field_p("os",1) + {"
</pre>
</body>
</html>"});
		return(deliver);
	}
	# json ua diag with optional jsonp callback parameter
	else if(resp.status == 849){
		set resp.status = 200;
		set req.http.dclass_cb = "";
		set req.http.dclass_cbe = "";
		if(req.url ~ "[^\w]callback=\w+"){
			set resp.http.Content-Type = "application/javascript; charset=utf-8";
			set req.http.dclass_cb = regsub(req.url,"^.*[^\w]callback=(\w+).*","\1(");
			set req.http.dclass_cbe = ");";
		} else {
			set resp.http.Content-Type = "application/json; charset=utf-8";
		}
		synthetic(
req.http.dclass_cb + {"{
"user_agent":""} + req.http.User-Agent + {"",
"dclass_version":""} + dclass.get_version() + {"",
"devicemap_id":""} + req.http.dclass_devicemap + {"",
"devicemap_type":""} + req.http.dclass_type + {"",
"devicemap_comment":""} + dclass.get_comment() + {"",
"devicemap_device":""} + dclass.get_field("vendor") + {" "} + dclass.get_field("model") + {"",
"devicemap_display":""} + dclass.get_ifield("displayWidth") + {"x"} + dclass.get_ifield("displayHeight") + {"",
"devicemap_input":""} + dclass.get_field("inputDevices") + {"",
"devicemap_js":""} + dclass.get_field("ajax_support_javascript") + {"",
"devicemap_wireless":""} + dclass.get_field("is_wireless_device") + {"",
"devicemap_tablet":""} + dclass.get_field("is_tablet") + {"",
"devicemap_crawler":""} + dclass.get_field("is_crawler") + {"",
"devicemap_desktop":""} + dclass.get_field("is_desktop") + {"",
"browser_id":""} + req.http.dclass_browser + {"",
"browser_comment":""} + dclass.get_comment_p(1) + {"",
"browser_name":""} + dclass.get_field_p("browser",1) + {"",
"browser_ver":""} + dclass.get_ifield_p("version",1) + {"",
"browser_os":""} + dclass.get_field_p("os",1) + {""
}"} + req.http.dclass_cbe);
		return(deliver);
	}
}

