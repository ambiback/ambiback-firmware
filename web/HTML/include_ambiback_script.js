/* Ambiback Javascripts */

window.ab = [];
window.ab.d = false;

function hCR(responseTxt, statusTxt, xhr){
	if (statusTxt == "success") {
/* TODO				var responseAsObject = $.parseJSON(responseTxt);
		$(this).html( responseAsObject.responseTxt );*/
		$(this).html( responseTxt );
	}
	if (statusTxt == "error") {
		$(this).html("Error: " + xhr.status + ": " + xhr.statusText);
		
		sPM('Failed to save setting to controller. <br>Details: ' + xhr.status + ": " + xhr.statusText, "Error"); 
	}
};

function hCRPreset(responseTxt, statusTxt, xhr){
	if (statusTxt == "success") {
		$(this).html( "<script>" + responseTxt + "</script>" );
	}
	if (statusTxt == "error") {
		$(this).html(	"<script>" +
						"console.log(\"Request Failed: " + xhr.status + "/" + xhr.statusText + "\");" +
						"</script>");
		sPM('Failed to save preset setting to controller. <br>Details: ' + xhr.status + ": " + xhr.statusText, "Error"); 
	}
};

function hCRAndReboot(responseTxt, statusTxt, xhr){
	if (statusTxt == "success") {
/* TODO				var responseAsObject = $.parseJSON(responseTxt);
		$(this).html( responseAsObject.responseTxt );*/
		$(this).html( responseTxt );
		
		$( "#result" ).load( "reboot?forreal=1", hCR );
		sL('System Variable changed, AmbiBack rebooting  ... please wait');
		tR(5000);
	}
	if (statusTxt == "error") {
		$(this).html("Error: " + xhr.status + ": " + xhr.statusText);

		sPM('Failed to save setting to controller. <br>Details: ' + xhr.status + ": " + xhr.statusText, "Error"); 
	}
};

function rP() {
	var curPageID = $( ":mobile-pagecontainer" ).pagecontainer( "getActivePage" ).prop("id");
	$( ":mobile-pagecontainer" ).pagecontainer( "change", "#" + curPageID, { 
	allowSamePageTransition : true,
	});
}

function hCS( event, ui ) {
	var nColorRed = Number($( "#ambient_slider_colorRed" ).val());
	var nColorGreen = Number($( "#ambient_slider_colorGreen" ).val());
	var nColorBlue = Number($( "#ambient_slider_colorBlue" ).val());
	var szColor =	(nColorRed < 16 ? "0" : "") + nColorRed.toString(16).toUpperCase() + 
					(nColorGreen < 16 ? "0" : "") + nColorGreen.toString(16).toUpperCase() + 
					(nColorBlue < 16 ? "0" : "") + nColorBlue.toString(16).toUpperCase();
	$( "#ambient_slider_preview" ).css('background-color', '#' + szColor);
	$( "#result" ).load( "setcolor?color=" + "FF" + szColor, hCR );
}

function tR(timeoutPeriod) {
	setTimeout("location.reload(true);",timeoutPeriod);
}
// Alias for backward compatibility with HTML
var timedRefresh = tR;

function rT(tabIdStr) {
  // Remove the panel
  $( tabIdStr ).remove();
  
  // Refresh the tabs widget
  $('#tabs').tabs( "refresh" );
  
  // Remove the Tab Link
  removeLi(tabIdStr); 
}

function gTIB( searchedId )
{
	var index = -1;
	var i = 0, els = $("#tabs").find("a");
	var l = els.length, e;
	while ( i < l && index == -1 ) {
		e = els[i];
		if ( "#" + searchedId == $(e).attr('href') ) {
			index = i;
		}
		i++;
	};
	return index;
}

function rL(liIdStr) {
	// Remove the li entry
	var hrefStr = "a[href='" + liIdStr + "']";
	$( hrefStr ).closest("li").remove();
  
	// Reorder Navbar Tabs
	$("#tabs-navbar > ul > li").each(function( index ) {
		console.log ( "removeLi reorder index " + index); 
		switch( index % 3) {
			case 0:
				$(this).removeClass("ui-block-b");
				$(this).removeClass("ui-block-c");
				$(this).removeClass("ui-block-d");
				$(this).addClass("ui-block-a");
				break;
			case 1:
				$(this).removeClass("ui-block-a");
				$(this).removeClass("ui-block-c");
				$(this).removeClass("ui-block-d");
				$(this).addClass("ui-block-b");
				break;
			case 2:
				$(this).removeClass("ui-block-a");
				$(this).removeClass("ui-block-b");
				$(this).removeClass("ui-block-d");
				$(this).addClass("ui-block-c");
				break;
			case 3:
				$(this).removeClass("ui-block-a");
				$(this).removeClass("ui-block-b");
				$(this).removeClass("ui-block-c");
				$(this).addClass("ui-block-d");
				break;
		}
	});      
}

function sPM(MessageText, MessageType) {
	if (MessageType == "Error") {
		$("#popupMessageLabel").removeClass("btn-green");
		$("#popupMessageLabel").removeClass("btn-yellow");
		$("#popupMessageLabel").addClass("btn-red");
		$("#popupMessageLabel").html("Error");
	} else if (MessageType == "Warn") {
		$("#popupMessageLabel").removeClass("btn-green");
		$("#popupMessageLabel").removeClass("btn-red");
		$("#popupMessageLabel").addClass("btn-yellow");
		$("#popupMessageLabel").html("Warning");
	} else if (MessageType == "Info") {
		$("#popupMessageLabel").removeClass("btn-red");
		$("#popupMessageLabel").removeClass("btn-yellow");
		$("#popupMessageLabel").addClass("btn-green");
		$("#popupMessageLabel").html("Information");
	}
	$("#popupMessageText").html( MessageText);
	$("#popupMessage").popup("open");
}

function cFU() {
	if (typeof window.ab !== 'undefined') {
		console.log('Check Update from: ' + window.ab.ubu + "/index.json");
		$( "#result" ).load( window.ab.ubu + "/index.json", hUR );
	}
}

function hUR(responseTxt, statusTxt, xhr){
	if (statusTxt == "success") {
		// Get response 
		var responseAsObject = $.parseJSON(responseTxt);
		console.log('hUR: ' + JSON.stringify(responseAsObject));
		console.log(responseAsObject.build + ">" + window.ab.build);
		console.log(responseAsObject.major + "." + responseAsObject.minor + "." + responseAsObject.build); 

		// Set variables
		var firmwareInfo = "Latest Firmware is " + responseAsObject.major + "." + responseAsObject.minor + "." + responseAsObject.build + ".</br>"; 
		var forceupdate = (typeof responseAsObject.forceupdate !== 'undefined' ? responseAsObject.forceupdate : 0);
		var allowdowngrade = (typeof responseAsObject.allowdowngrade !== 'undefined' ? responseAsObject.allowdowngrade : 0);
		var allowbeta = (typeof responseAsObject.allowbeta !== 'undefined' ? responseAsObject.allowbeta : 0);
		
		
		window.ab.uv = responseAsObject.major + "." + responseAsObject.minor + "." + responseAsObject.build
		console.log("forceupdate: " + forceupdate + ", allowdowngrade" + allowdowngrade); 
		
		// Add default Firmware Version entry!
		$("#firmware_version").append(new Option("Latest Version " + window.ab.uv + (responseAsObject.build == window.ab.build ? " (installed)" : ""), window.ab.uv));
		$("#firmware_version").val(window.ab.uv).selectmenu("refresh");
		
		if (allowdowngrade == 1 && typeof responseAsObject.downgradebuilds !== 'undefined') {
			var downgradebuilds = responseAsObject.downgradebuilds;
			$.each(downgradebuilds, function(keyIn, oldbuild){
				if (typeof oldbuild.major !== 'undefined' && 
					typeof oldbuild.minor !== 'undefined' &&
					typeof oldbuild.build !== 'undefined') {
					var downgradeversion = oldbuild.major + "." + oldbuild.minor + "." + oldbuild.build;
					console.log("downgradeversion: " + downgradeversion);
					$("#firmware_version").append(new Option("Old Version " + downgradeversion + (oldbuild.build == window.ab.build ? " (installed)" : "(downgrade)"), downgradeversion));
				}
			});
		}
		if (allowbeta == 1 && typeof responseAsObject.betabuilds !== 'undefined') {
			var betabuilds = responseAsObject.betabuilds;
			$.each(betabuilds, function(keyIn, betabuild){
				if (typeof betabuild.major !== 'undefined' && 
					typeof betabuild.minor !== 'undefined' &&
					typeof betabuild.build !== 'undefined') {
					var betaversion = betabuild.major + "." + betabuild.minor + "." + betabuild.build;
					console.log("betaversion: " + betaversion);
					$("#firmware_version").append(new Option("BETA Version " + betaversion + (betabuild.build == window.ab.build ? " (installed)" : "(UNSTABLE)"), betaversion));
				}
			});
		}
		
		

		// Check if update is available
		if (responseAsObject.build > window.ab.build || forceupdate == 1) {
			firmwareInfo += "Click Update to install.";
			$('#firmware_update').removeClass("ui-disabled");
			$('#firmware_update').addClass("btn-red");
		} else if (responseAsObject.build == window.ab.build || forceupdate == 1) {
			$('#firmware_update').removeClass("ui-disabled");
			$('#firmware_update').removeClass("btn-red");
		} else {
			$('#firmware_update').removeClass("btn-red");
			if ( allowdowngrade == 0) {
				// Only disable if downgrade not allowed
				$('#firmware_update').addClass("ui-disabled");
			}
		}
		$( "#firmware_info" ).html(firmwareInfo);
		console.log(firmwareInfo);
	}
	if (statusTxt == "error") {
		console.log("hUR Error: " + xhr.status + ": " + xhr.statusText);
	}
};

function hDRS(responseTxt, statusTxt, xhr){
	if (statusTxt == "success") {
		// Get response 
		var responseAsObject = $.parseJSON(responseTxt);
		console.log('hDRS: ' + JSON.stringify(responseAsObject));
		
		$( "#result" ).load( "setparam?param=setdebugsyslogserver&value=" + responseAsObject.ip, hCR );
		$( "#result" ).load( "setparam?param=setdebugsyslog&value=1", hCR );
		$( "#result" ).load( "setparam?param=setdebugserial&value=0", hCR );
		
		$('#btn_system_debug').removeClass("ui-icon-check");
		$('#btn_system_debug').addClass("ui-icon-forbidden");
		$('#btn_system_debug').text("Disable Remote Debug"); 
		$('#btn_system_debug').addClass("btn-green");
	}
	if (statusTxt == "error") 
		console.log("hDRS Error: " + xhr.status + ": " + xhr.statusText);
}

function hSUR(responseTxt, statusTxt, xhr){
	if (statusTxt == "success") {
		// Get response 
		var responseAsObject = $.parseJSON(responseTxt);
		console.log('hSUR: ' + JSON.stringify(responseAsObject));
		var modestatus = (typeof responseAsObject.modestatus !== 'undefined' ? responseAsObject.modestatus : "");
		var videostatus = (typeof responseAsObject.videostatus !== 'undefined' ? responseAsObject.videostatus : "");
		var huestatus = (typeof responseAsObject.huestatus !== 'undefined' ? responseAsObject.huestatus : "");
		var dreamscreenstatus = (typeof responseAsObject.dreamscreenstatus !== 'undefined' ? responseAsObject.dreamscreenstatus : "");
		var dreamscreenfps = (typeof responseAsObject.dreamscreenfps !== 'undefined' ? responseAsObject.dreamscreenfps : 0);
		var wifinetwork = (typeof responseAsObject.wifinetwork !== 'undefined' ? responseAsObject.wifinetwork : "");
		var wifirssi = (typeof responseAsObject.wifirssi !== 'undefined' ? responseAsObject.wifirssi : 0);
		var ledfps = (typeof responseAsObject.ledfps !== 'undefined' ? responseAsObject.ledfps : 0);
		var netfps = (typeof responseAsObject.netfps !== 'undefined' ? responseAsObject.netfps : 0);
		var netdrpfps = (typeof responseAsObject.netdrpfps !== 'undefined' ? responseAsObject.netdrpfps : 0);
		var camfps = (typeof responseAsObject.camfps !== 'undefined' ? responseAsObject.camfps : 0);
		var huefps = (typeof responseAsObject.huefps !== 'undefined' ? responseAsObject.huefps : 0);
		var blkscnds = (typeof responseAsObject.blkscnds !== 'undefined' ? responseAsObject.blkscnds : 0);

		var freemem = (typeof responseAsObject.freemem !== 'undefined' ? responseAsObject.freemem : 0);
		var freeheap = (typeof responseAsObject.freeheap !== 'undefined' ? responseAsObject.freeheap : 0);
		var freepsram = (typeof responseAsObject.freepsram !== 'undefined' ? responseAsObject.freepsram : 0);
		var cpufreq = (typeof responseAsObject.cpufreq !== 'undefined' ? responseAsObject.cpufreq : 0);
		var cpuxtalfreq = (typeof responseAsObject.cpuxtalfreq !== 'undefined' ? responseAsObject.cpuxtalfreq : 0);
		var cpunumtasks = (typeof responseAsObject.cpunumtasks !== 'undefined' ? responseAsObject.cpunumtasks : 0);

		var camavrout = (typeof responseAsObject.camavrout !== 'undefined' ? responseAsObject.camavrout : 0);
		var camavrin = (typeof responseAsObject.camavrin !== 'undefined' ? responseAsObject.camavrin : 0);
		var camaecvalue = (typeof responseAsObject.camaecvalue !== 'undefined' ? responseAsObject.camaecvalue : 0);
		var camagcgain = (typeof responseAsObject.camagcgain !== 'undefined' ? responseAsObject.camagcgain : 0);

		$('#status_wifi').text("Wifi: " + wifinetwork);
		$('#status_wifisignal').text("Wifi: " + wifirssi + "dbm");
		$('#status_video').text("Network Input: " + videostatus + "(FPS/DRP: " + netfps + "/" + netdrpfps + ")");
		$('#status_hue').text("Hue Stream: " + huestatus + "(FPS:" + huefps + ")");
		$('#status_dreamScreen').text("DreamScreen: " + dreamscreenstatus + "(FPS:" + dreamscreenfps + ")");
		$('#status_fps').text("LED FPS: " + ledfps);
		$('#status_sys').text("CPU: " + cpufreq + "Mhz, Free Mem/Heap/PSRam: " + Math.round(freemem/1024) + "/" + Math.round(freeheap/1024) + "/" + Math.round(freepsram/1024));
		$('#status_cam').text("CAM FPS/Black: " + camfps + "/" + (blkscnds > 84600 ? Math.round(blkscnds / 84600) + "d, " + dD(Math.round(blkscnds / 3600)) + ":" + dD(Math.round(blkscnds % 3600 / 60)) + ":" + dD(blkscnds % 3600 % 60) : dD(Math.round(blkscnds / 3600)) + ":" + dD(Math.round(blkscnds % 3600 / 60)) + ":" + dD(blkscnds % 60)) + " (h:m:s)");
		$('#status_cam_xtr').text("AvrOut: " + camavrout + ", AvrIn: " + camavrin + ", Aec: " + camaecvalue + ", Gain: " + camagcgain);

		var modeid = "mode-off"; 
		if (modestatus == "Video")
			modeid = "mode-video"; 
		else if (modestatus == "Ambient")
			modeid = "mode-ambient"; 
		else if (modestatus == "Cam")
			modeid = "mode-cam"; 
		else if (modestatus == "Relay")
			modeid = "mode-relay"; 
		
		// Use CSS Method to set button active state
		var modebuttons = $("#mode-control").find("a"); 
		for(var i = 0; i < modebuttons.length; i++) {
			if (modeid != modebuttons[i].id && 
				$("#" + modebuttons[i].id).hasClass('ui-btn-active')) { 
				console.log("Disable SELECT for " + modebuttons[i].id);
				$("#" + modebuttons[i].id ).removeClass('ui-btn-active');
			}
		}

		// Add active for selected MODE only!
		$("#" + modeid).addClass('ui-btn-active'); 
		
		// Wifi Signal Strengtrh
		if (wifirssi < 0) {
			if (wifirssi > -65) {
				$('#status_wifisignal').removeClass("btn-red");
				$('#status_wifisignal').removeClass("btn-blue");
				$('#status_wifisignal').removeClass("btn-orange");
				$('#status_wifisignal').addClass("btn-green");
			} else if (wifirssi > -80) {
				$('#status_wifisignal').removeClass("btn-red");
				$('#status_wifisignal').removeClass("btn-blue");
				$('#status_wifisignal').removeClass("btn-green");
				$('#status_wifisignal').addClass("btn-orange");
			} else {
				$('#status_wifisignal').removeClass("btn-orange");
				$('#status_wifisignal').removeClass("btn-blue");
				$('#status_wifisignal').removeClass("btn-green");
				$('#status_wifisignal').addClass("btn-red");
			}
		} else {
			$('#status_wifisignal').removeClass("btn-red");
			$('#status_wifisignal').removeClass("btn-green");
			$('#status_wifisignal').removeClass("btn-orange");
			$('#status_wifisignal').addClass("btn-blue");
		}
		if (freemem > 81920) {
			$('#status_sys').removeClass("btn-red");
			$('#status_sys').removeClass("btn-orange");
			$('#status_sys').addClass("btn-green");
		} else if (freemem > 40960) {
			$('#status_sys').removeClass("btn-red");
			$('#status_sys').removeClass("btn-green");
			$('#status_sys').addClass("btn-orange");
		} else if (freemem > 0) {
			$('#status_sys').removeClass("btn-green");
			$('#status_sys').removeClass("btn-orange");
			$('#status_sys').addClass("btn-red");
		}

		if (videostatus == "connected") {
			$('#status_video').removeClass("btn-red");
			$('#status_video').addClass("btn-green");
		} else {
			$('#status_video').removeClass("btn-green");
			$('#status_video').addClass("btn-red");
		}
		if (huestatus == "connected") {
			$('#status_hue').removeClass("btn-red");
			$('#status_hue').removeClass("btn-orange");
			$('#status_hue').addClass("btn-green");
		} else if (huestatus == "prepared") {
			$('#status_hue').removeClass("btn-red");
			$('#status_hue').removeClass("btn-green");
			$('#status_hue').addClass("btn-orange");
		} else if (huestatus == "disconnected") {
			$('#status_hue').removeClass("btn-green");
			$('#status_hue').removeClass("btn-orange");
			$('#status_hue').addClass("btn-red");
		}
		if (dreamscreenstatus == "connected") {
			$('#status_dreamScreen').removeClass("btn-red");
			$('#status_dreamScreen').addClass("btn-green");
		} else {
			$('#status_dreamScreen').removeClass("btn-green");
			$('#status_dreamScreen').addClass("btn-red");
		}
		
		if ( window.sC !== 'undefined' ) {
			// Set Title
			$('#status_connections').text("Connections (Check " + window.sCsecs + ") ..."); 
			
			// Check Status again
			if (window.sC == true) {
				console.log("hSUR Check again in 1000ms");
				// Sleep for a second
				setTimeout(function() {
					// Increment counter 
					window.sCsecs++;
					
					// Check again
					$( "#result" ).load( "getstatusasjson", hSUR );
				}, 1000);
				/*
				// Sleep for a second
				sleep(1000).then(() => {
					// Increment counter 
					window.sCsecs++;
					
					// Check again
					$( "#result" ).load( "getstatusasjson", hSUR );
				});
				/**/
			}
		}
		
	}
	if (statusTxt == "error") 
		console.log("hSUR Error: " + xhr.status + ": " + xhr.statusText);
}

function fSN(){
	var allsliders = document.getElementsByClassName('ui-slider');
	console.log("Found sliders:" + allsliders.length); 
	for(var i = 0; i < allsliders.length; i++) {
		//Remove the 'active' class from the element.  
		//This will automatically update the nodeList's length too.
		var className = allsliders[i].getAttribute('class').replace('active','');
		if(typeof className.includes === "function"){
			if (!className.includes("notouchscroll")) {
				allsliders[i].setAttribute('class', className + " notouchscroll");		
				// console.log("Add Class to Slider " + i + ": " + className + " notouchscroll" ); 
			}
		}
	}
}

function hL() {
	// Hide loading Window
	$.mobile.loading('hide');
}

function sL(szText) {
	$.mobile.loading( 'show', {
			text: szText,
			textVisible: true,
			theme: 'b',
			html: ""
		});
}

function iAD() {
	
	$("#system_relayto").empty(); 
	$("#system_relayto").append(new Option("--This device--", ""));
	$("#system_relayto").val("--This device--");
	$("#system_relayto").selectmenu("refresh");
	// Init from window variables
	if (typeof window.ab !== 'undefined' &&
		typeof window.ab.dev !== 'undefined' && 
		typeof window.ab.dev.cnt !== 'undefined') {
		for	(var i = 0; i < window.ab.dev.cnt; i++) {
			$("#system_relayto").append(new Option(window.ab.dev.name[i] + " (" + window.ab.dev.ip[i] + ")", window.ab.dev.mac[i]));
		}	
	}
	// Select configured DsGroup!
	if (typeof window.ab !== 'undefined' &&
		typeof window.ab.selecteddevice !== 'undefined') {
		$("#system_relayto").val(window.ab.selecteddevice).selectmenu("refresh");
	}
}

function hRTC(szRelay) {
	if (szRelay != null && szRelay.length > 0) {
		$("#system_relayto_direct").checkboxradio("enable");
		
		var isdirect = ($("#system_relayto_direct").prop("checked") ? 1 : 0);
		if (isdirect == 1) {
			// Disable COLOR Tab on disabled!
			$("#tabs").tabs({disabled: [gTIB("tabs-setup"), gTIB("tabs-color")]});
		} else 
			$("#tabs").tabs({disabled: [gTIB("tabs-setup")]});
	} else {
		$("#tabs").tabs({disabled: []});
		$("#system_relayto_direct").checkboxradio("disable");
	}
}
// Alias for backward compatibility with HTML
var handleRelayToChanged = hRTC;

function dD(n){
    return n > 9 ? "" + n: "0" + n;
}

/*
// sleep time expects milliseconds
function sleep(time) {
	return new Promise((resolve) => setTimeout(resolve, time));
}
*/

