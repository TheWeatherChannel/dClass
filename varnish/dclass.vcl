import dclass;

backend default {
	.host = "localhost";
	.port = "80";
}

sub vcl_init {
	# load openddr
	dclass.init_dclass("/some/path/OpenDDR/1.0.0.4/resources");
	#dclass.init_dclass_p("/some/path/OpenDDR/1.0.0.4/resources",0);

	# load browser dtree
	dclass.init_dclass_p("/some/path/dClass/dtrees/browser.dtree",1);
}

sub vcl_recv {
	# do the lookup
	set req.http.dclass_openddr = dclass.classify(req.http.user-agent);
	#set req.http.dclass_openddr = dclass.classify_p(req.http.user-agent,0);
	set req.http.dclass_browser = dclass.classify_p(req.http.user-agent,1);

	if(req.url == "/uadiag"){
		error 849 "uadiag";
	}
}

sub vcl_error {
	if(obj.status == 849){
		set obj.status = 200;
		set obj.http.Content-Type = "text/html; charset=utf-8";
		synthetic
{"<html><title>uadiag</title>
<body>
<h1>UADIAG</h1>
<pre>
User-Agent:       "} + req.http.User-Agent + {"

OpenDDR id:       "} + req.http.dclass_openddr + {"
OpenDDR device:   "} + dclass.get_field("vendor") + {" "} + dclass.get_field("model") + {"
OpenDDR display:  "} + dclass.get_ifield("displayWidth") + {"x"} + dclass.get_ifield("displayHeight") + {"
OpenDDR js:       "} + dclass.get_field("ajax_support_javascript") + {"
OpenDDR wireless: "} + dclass.get_field("is_wireless_device") + {"
OpenDDR tablet:   "} + dclass.get_field("is_tablet") + {"

browser id:       "} + req.http.dclass_browser + {"
browser name:     "} + dclass.get_field_p("browser",1) + {"
browser ver:      "} + dclass.get_ifield_p("version",1) + {"
browser os:       "} + dclass.get_field_p("os",1) + {"
</pre>
</body>
</html>"};
		return(deliver);
	}
}

