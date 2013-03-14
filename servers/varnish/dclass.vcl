import dclass;

backend default {
	.host = "localhost";
	.port = "80";
}

sub vcl_init {
	# load openddr 
	dclass.init_dclass("/some/path/dClass/dtrees/openddr.dtree");
	#dclass.init_dclass_p("/some/path/OpenDDR/1.0.0.0/resources",0);

	# load browser dtree
	dclass.init_dclass_p("/some/path/dClass/dtrees/browser.dtree",1);
}

sub vcl_recv {
	# ua override
	if(req.url ~ "[^\w]ua="){
		# TODO this needs to be URL decoded...
		set req.http.user-agent = regsub(req.url,"^.*[^\w]ua=([^&]*).*","\1");
	}

	# do the lookup
	set req.http.dclass_openddr = dclass.classify(req.http.user-agent);
	#set req.http.dclass_openddr = dclass.classify_p(req.http.user-agent,0);
	set req.http.dclass_browser = dclass.classify_p(req.http.user-agent,1);

	if(req.url == "/uadiag"){
		error 848 "uadiag";
	} else if(req.url ~ "^/uadiag.js"){
		error 849 "uadiag.js";
	}
}

sub vcl_error {
	# plain text ua diag
	if(obj.status == 848){
		set obj.status = 200;
		set obj.http.Content-Type = "text/html; charset=utf-8";
		synthetic
{"<html><title>UADIAG</title>
<body>
<h1>UADIAG</h1>
<pre>
User-Agent:       "} + req.http.User-Agent + {"

dClass version:   "} + dclass.get_version() + {"

OpenDDR id:       "} + req.http.dclass_openddr + {"
OpenDDR device:   "} + dclass.get_field("vendor") + {" "} + dclass.get_field("model") + {"
OpenDDR display:  "} + dclass.get_ifield("displayWidth") + {"x"} + dclass.get_ifield("displayHeight") + {"
OpenDDR js:       "} + dclass.get_field("ajax_support_javascript") + {"
OpenDDR wireless: "} + dclass.get_field("is_wireless_device") + {"
OpenDDR tablet:   "} + dclass.get_field("is_tablet") + {"
OpenDDR crawler:  "} + dclass.get_field("is_crawler") + {"
OpenDDR desktop:  "} + dclass.get_field("is_desktop") + {"

browser id:       "} + req.http.dclass_browser + {"
browser name:     "} + dclass.get_field_p("browser",1) + {"
browser ver:      "} + dclass.get_ifield_p("version",1) + {"
browser os:       "} + dclass.get_field_p("os",1) + {"
</pre>
</body>
</html>"};
		return(deliver);
	}
	# json ua diag with optional jsonp callback parameter
	else if(obj.status == 849){
		set obj.status = 200;
		set req.http.dclass_cb = "";
		set req.http.dclass_cbe = "";
		if(req.url ~ "[^\w]callback=\w+"){
			set obj.http.Content-Type = "application/javascript; charset=utf-8";
			set req.http.dclass_cb = regsub(req.url,"^.*[^\w]callback=(\w+).*","\1(");
			set req.http.dclass_cbe = ");";
		} else {
			set obj.http.Content-Type = "application/json; charset=utf-8";
		}
		synthetic
req.http.dclass_cb + {"{
"user_agent":""} + req.http.User-Agent + {"",
"dclass_version":""} + dclass.get_version() + {"",
"openddr_id":""} + req.http.dclass_openddr + {"",
"openddr_device":""} + dclass.get_field("vendor") + {" "} + dclass.get_field("model") + {"",
"openddr_display":""} + dclass.get_ifield("displayWidth") + {"x"} + dclass.get_ifield("displayHeight") + {"",
"openddr_js":""} + dclass.get_field("ajax_support_javascript") + {"",
"openddr_wireless":""} + dclass.get_field("is_wireless_device") + {"",
"openddr_tablet":""} + dclass.get_field("is_tablet") + {"",
"openddr_crawler":""} + dclass.get_field("is_crawler") + {"",
"openddr_desktop":""} + dclass.get_field("is_desktop") + {"",
"browser_id":""} + req.http.dclass_browser + {"",
"browser_name":""} + dclass.get_field_p("browser",1) + {"",
"browser_ver":""} + dclass.get_ifield_p("version",1) + {"",
"browser_os":""} + dclass.get_field_p("os",1) + {""
}"} + req.http.dclass_cbe;
		return(deliver);
	}
}

