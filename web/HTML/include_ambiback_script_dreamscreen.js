/* Ambiback DreamScreen Javascripts */
window.dsV = [];
window.dsV.ADRT = 30;

// --- UNCOMMENT FOR Debug
/*	
*
// window.hueVars = [];
window.dsG = [];
window.dsG.ids = []; 
window.dsG.gn = []; 
window.dsG.ids[0] = 0; 
window.dsG.ids[1] = 1; 
window.dsG.gn[0] = "Wohnen"; 
window.dsG.gn[1] = "Schlafen"; 
window.dsG.gc = 2; 
// --- */

function iDST() {
	// Init the fucking TAB!
	$("#tabs").trigger('create');

	// Enable/disable Buttons
//	$('#smart_RefreshDreamScreenDevices').addClass("ui-disabled");
	
	// --- DreamScreen Settings
	// Init Checkboxes
	$("#dreamscreen_emulation").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setdreamscreenemulation&value=" + ischecked, hCR );
		
		if (ischecked == 1) { 
			$('#div_dreamscreen_emulation').find("label, input").addClass("btn-checked");
		} else {
			$('#div_dreamscreen_emulation').find("label, input").removeClass("btn-checked");
		}
		
	});
	
	// Init DreamScreen Groups
	iDSDOC(); // Calls iDSG() automatically  

	$("#dreamscreen_group").on( "change", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setdreamscreengroup&value=" + $(this).val(), hCR );
	});
	
	// Init Buttons
	$('#smart_RefreshDreamScreenDevices').on('click', function (e) {
		$.mobile.loading( 'show', { 
			text: "Searching DreamScreen Groups ... please wait.", 
			textVisible: true, 
			theme: 'b', 
			html: ''});
		
		// Trigger Init on controller
		$( "#result" ).load( "refresh_dreamscreen_devices", hDDR );
	});
	
	$('#smart_ScanForDreamScreenDevices').on('click', function (e) {
		$.mobile.loading( 'show', { 
			text: "Scanning for new DreamScreen devices ... please wait.", 
			textVisible: true, 
			theme: 'b', 
			html: ''});
		
		// Trigger Init on controller
		$( "#result" ).load( "getscannetworksasjson", hSSR );
	});
	
	// Init Sliders
	$( "#dreamscreen_lowcolorsmoothing" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setdreamscreencolorsmoothing&value=" + $(this).val(), hCR );
	});

	// New - refresh devices on DreamScreen Init! 
	$( "#result" ).load( "refresh_dreamscreen_devices", hDDR );
	
	// Set Tab Loaded
	window.tVars['link_dreamscreen'] = true;
	
	// Fix Sliders Scrolling issue
	fSN();
	
	// Hide Loading form!
	hL();
}

function iDSG() {
	$.mobile.loading('hide');
	
	$("#dreamscreen_group").empty(); 
	$("#dreamscreen_group").append(new Option("--select--", "select"));
	$("#dreamscreen_group").val("select");
	$("#dreamscreen_group").selectmenu("refresh");
	// Init from window variables
	if (typeof window.dsG !== 'undefined' && typeof window.dsG.gc !== 'undefined') {
		for	(var i = 0; i < window.dsG.gc; i++) {
			$("#dreamscreen_group").append(new Option(window.dsG.gn[i], window.dsG.ids[i]));
		}	
	}
	// Select configured DsGroup!
	if (typeof window.dsG.selectedgroup !== 'undefined') {
		$("#dreamscreen_group").val(window.dsG.selectedgroup).selectmenu("refresh");
	}
}

function iDSDOC() {
	// Reload Settings from controller
	var dateNow = new Date();
	$.getScript( "controllersettings_dreamscreen.js?"+dateNow.getTime() )
	  .done(function( script, textStatus ) {
		console.log( "iDSDOC: " + textStatus );

		// Init DreamScreen Groups
		iDSG(); 
	  })
	  .fail(function( jqxhr, settings, exception ) {
		$.mobile.loading('hide');
		console.log( "iDSDOC: Triggered ajaxError handler." );
		$( "#result" ).text( "Triggered ajaxError handler." );
	});
}

function hDDR(responseTxt, statusTxt, xhr){
	if (statusTxt == "success") {
		$(this).html( responseTxt );
		setTimeout(function() {
			// Get data delayed
			iDSDOC(); 
		}, 1000);
	} else if (statusTxt == "error") {
		$.mobile.loading('hide');
		$(this).html("Error: " + xhr.status + ": " + xhr.statusText);
	}
};

function hSSR(responseTxt, statusTxt, xhr){
	$.mobile.loading('hide');
	if (statusTxt == "success") {
		// Get response 
		var responseAsObject = $.parseJSON(responseTxt);
		if ( typeof responseAsObject.networks !== 'undefined' ) {
			var networks = responseAsObject.networks;
			var foundfirst = true;
			
			// Append CSS to UI to overwrite jquery mobile CSS
/*			$('#dreamscreen_devices').append( '<style>' + 
			'.controlgroup-dreamscreen { float: left; } \n' + 
			'</style>' ); 
*/
			// empty old stuff first
			$("#dreamscreen_devices").html(""); 
			
			$.each(networks, function(keyNet, valueNet){
				$.each(valueNet, function(keyIn, valueIn){
					// console.log(keyIn + ":" + valueIn);
					if (keyIn == "SSID") {
						if (valueIn.includes("DreamScreen") || valueIn.includes("SideKick")) {
							window.aV = valueIn; 
		var hNID = valueIn.replace(/ |_/g, ""); 
		var dNID = 'dreamscreen_network_' + hNID;
							console.log("DreamScreen WIFI Found: " + valueIn);
							console.log("Adding WIFI " + valueIn + " to " + szDivNetworkID);
							
							if (foundfirst) {
								$("#dreamscreen_devices").append('<div data-role="header"><h2>New DreamScreen Devices</h2></div>'); 
								foundfirst = false;
							}
							
							var h = `
							<div class="ui-field-contain">
								<label for=${szDivNetworkID}"_name">Found device:</label>
								<div data-role="controlgroup" data-type="horizontal" id=${szDivNetworkID}>
									<input type="text" disabled="disabled" name="${szDivNetworkID}_name" id="${szDivNetworkID}_name" value="${valueIn}" data-wrapper-class="controlgroup-dreamscreen">
									<button id="${szDivNetworkID}_addtonetwork" class="controlgroup-dreamscreen_button">Add to network</button>
								</div>						
							</div>
							`;
							// Append UI
							$("#dreamscreen_devices").append( szHtml );
							
							// Init JQuery by calling create on parent element
							$("#" + szDivNetworkID).trigger('create');
							
							// Bind Button event
							$( "#" + szDivNetworkID + "_addtonetwork" ).on( "click", function (e) {
								console.log("addtonetwork: " + szDivNetworkID);
								window.aS = new Date();
								
								$.mobile.loading( 'show', { 
									text: "Adding " + valueIn + " to your network ... please wait", 
									textVisible: true, 
									theme: 'b', 
									html: ''});
								
								$( "#result" ).load( "dreamscreenaddtonetwork?value=" + encodeURI(valueIn), hADR );
							});
							
						} else {
							console.log("WIFI Found: " + valueIn);
						}
					}
				});
				
			});			
		} else {
			console.log('hSSR: MISSING FIELDS: ' + JSON.stringify(responseAsObject));
		}
	}
	if (statusTxt == "error") 
		console.log("hSSR Error: " + xhr.status + ": " + xhr.statusText);
}

function hADR(responseTxt, statusTxt, xhr){
	var dateNow = new Date();
	var timeRun = Math.round((dateNow - window.actionstarted) / 1000);
	
	if (statusTxt == "success") {
		// Get response 
		var responseAsObject = $.parseJSON(responseTxt);
		console.log('hADR: ' + JSON.stringify(responseAsObject));

		var Status = (typeof responseAsObject.status !== 'undefined' ? responseAsObject.status : null);
		var Action = (typeof responseAsObject.action !== 'undefined' ? responseAsObject.action : null);
		var Device = (typeof responseAsObject.device !== 'undefined' ? responseAsObject.device : null);
		
		if ( Status != null && Action != null && Action == "AddDreamScreenToNetwork" ) {
			$.mobile.loading('hide');
			if (Status == 1) {
				sPM('Successfully added Dreamscreen device "' + Device + '" to your local network.', "Info"); 
			} else {
				sPM('Failed adding dreamscreen device "' + Device + '" to local network. <br>Status: ' + Status, "Error"); 
			}
			return; 
		} 
	} else if (statusTxt == "error") {
		console.log('hADR: Failed getting result with ' + xhr.status + ": " + xhr.statusText);
	}
	
	if (timeRun < window.dsV.ADRT) {
		// Continue to check
		setTimeout(function() {
			// get update status
			var dateNow = new Date();
			var szLoading = "Adding " + window.aV + " to your network ... please wait " + (timeRun > 0 ? timeRun : "..."); 
			$.mobile.loading( 'show', { text: szLoading, textVisible: true, theme: 'b'});
			$( "#result" ).load( "getlaststatusmessageasjson?" + dateNow.getTime(), hADR );
		}, 1000);
	} else {
		$.mobile.loading('hide');
		sPM('Timeout ' + window.dsV.ADRT + ' adding dreamscreen device "' + Device + '" to local network.', "Error"); 
	}
}
