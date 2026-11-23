/* Ambiback Camera Javascripts */

// Hue objects
var h = jsHue();
var hB = null;
var hU = null; 
var hLights = null; 
var hZ = 12; 
// --- UNCOMMENT FOR Debug
//window.hIP = '';
//window.hUsr = '';


// Enums
var i=1;
var ehp = {
	ultra: i++,
	fast: i++,
	normal: i++,
	custom: 999,
}

function dE(el) {
	for (var i = 0; i < el.length; i++) {
		el[i].addClass("ui-disabled");
///		el[i].disabled = true;
		dE(el[i].children);
	}
}

function eE(el) {
	for (var i = 0; i < el.length; i++) {
		el[i].removeClass("ui-disabled");
//		el[i].disabled = false;
		eE(el[i].children);
	}
}

function iHT() {
	// Init the fucking TAB!
	$("#tabs").trigger('create');

	// Set HueIP/User
	$('#smart_Huebridge').val(window.hIP); 
	
	// --- Init Hue Bridge / Users 
	h.discover().then(bridges => {
		if(bridges.length === 0) {
			console.log('No bridges found. :(');
		}
		else {
			bridges.forEach(b => {
				console.log('Bridge found at IP address %s.', b.internalipaddress);
				if ($('#smart_Huebridge').val() == "") {
					$('#smart_Huebridge').val(b.internalipaddress);
				}
			});
		}
	}).catch(e => console.log('Error finding bridges', e));
	
	// Enable/disable Buttons
	$('#smart_ConnectHueBridge').removeClass("ui-disabled");
	$('#smart_DisconnectHueBridge').addClass("ui-disabled");
	$('#smart_RefreshHueDevices').addClass("ui-disabled");
	
	// Init HUE Access, if already configured
	if (window.hIP != "") {
		hB = h.bridge(window.hIP); 
		console.log('Init: Bridge obj: ', hB);
		if (window.hUsr != "") {
			hU = hB.user(window.hUsr);
			console.log('Init: User obj: ', hU);

			// Init Hue Lights
			hGL(); 
		}
	}
	
	// --- Hue Settings
	// Init Checkboxes
	$("#smart_hue_support").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=sethuesupport&value=" + ischecked, hCR );
		console.log("smart_hue_support change " + ischecked);
		console.log ( $('#ui_form_hue_devices').find("*") );
		if (ischecked == 1) {
			$('#ui_form_hue_devices').find("a, input").removeClass("ui-disabled");
			$('#div_smart_hue_support').find("label, input").addClass("btn-checked");
		} else {
			$('#ui_form_hue_devices').find("a, input").addClass("ui-disabled"); 
			$('#div_smart_hue_support').find("label, input").removeClass("btn-checked");
		}
	});
	
	// Init Sliders 
	$( "#slider_smart_FrameLength" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=sethueframelength&value=" + $(this).val(), hCR );
	});
	$( "#slider_smart_HandshakeTimeout" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=sethuehandshaketimeout&value=" + $(this).val(), hCR );
	});

	// Init BrightnessPresets
	$("#hue_Presets").append(new Option("--select--", "select"));
	$("#hue_Presets").val("select");
	$("#hue_Presets").selectmenu("refresh");
	$("#hue_Presets").append(new Option("Ultra (50fps)", ehp.ultra));
	$("#hue_Presets").append(new Option("Fast (35fps)", ehp.fast));
	$("#hue_Presets").append(new Option("normal (25fps)", ehp.normal));
	$("#hue_Presets").append(new Option("Custom", ehp.custom));
	$("#hue_Presets").on( "change", function( event, ui ) {
		console.log($(this).val());

		// Store preset number
		$( "#result" ).load( "setpreset?param=sethuepreset&value=" + $(this).val(), hCRPreset );

		/// Show hide custom settings
		if ($(this).val() == ehp.custom) {
			// Custom Mode, show Settings
			$("#hue_enhancement_custom").show();
		} else {
			// Hide Custom properties
			$("#hue_enhancement_custom").hide();
		}
	});

	
	// Init Buttons
	$('#smart_ConnectHueBridge').on('click', function (e) {
		// TODO make Hue Connection
		window.hIP = $('#smart_Huebridge').val(); 
		hB = h.bridge(window.hIP); 
		console.log ( "Hue ID: " + window.hAppID );
		
		// create user account (requires link button to be pressed)
		hB.createUser(window.hAppID).then(data => {
			console.log('DATA: ' + JSON.stringify(data));
			// extract bridge-generated username from returned data
			window.hUsr = data[0].success.username;
			window.hCk = data[0].success.clientkey;
			console.log('New username:', window.hUsr);
			console.log('New clientkey:', window.hCk);

			// instantiate user object with username
			hU = hB.user(window.hUsr);
			console.log('User obj: ', hU);
			
			// Store on controller
			$( "#result" ).load( "setparam?param=sethuebridgeip&value=" + window.hIP, hCR );
			$( "#result" ).load( "setparam?param=sethuebridgeuser&value=" + window.hUsr, hCR );
			$( "#result" ).load( "setparam?param=sethuebridgeclientkey&value=" + window.hCk, hCR );
			
			// Enable/disable Buttons
			$('#smart_ConnectHueBridge').addClass("ui-disabled");
			$('#smart_DisconnectHueBridge').removeClass("ui-disabled");
			$('#smart_RefreshHueDevices').removeClass("ui-disabled");
		});
	});
	
	$('#smart_DisconnectHueBridge').on('click', function (e) {
		// delete all objs
		hB = null;
		hU = null;
		hLights = null; 
		// Enable/disable Buttons
		$('#smart_ConnectHueBridge').removeClass("ui-disabled");
		$('#smart_DisconnectHueBridge').addClass("ui-disabled");
		$('#smart_RefreshHueDevices').addClass("ui-disabled");
		
		// Hide ConnectInfo
		$('#smart_hue_connectinfo').show();
		
		window.hIP = "";
		window.hUsr = "";
		// Store on controller ?
	});
	
	$('#smart_RefreshHueDevices').on('click', function (e) {
		// Get list of devices!
		if (hB != null) {
			console.log( "Enum Lights" ); 
			hGL();

			// Trigger Init on controller
			$( "#result" ).load( "refresh_hue_devices", hCR );
		}
	});
	
	// Hide Custom properties
	$("#hue_enhancement_custom").hide();

	// Set Tab Loaded
	window.tVars['link_hue'] = true;

	// Fix Sliders Scrolling issue
	fSN();
	
	// Hide Loading form!
	hL();
}
// Alias for backward compatibility with HTML
var initHueTab = iHT;

function hGL() {
	if (hU != null) {
		hU.getLights().then(data => {
			hLights = data; 
			
			// Append CSS to UI to overwrite jquery mobile CSS
			$('#smart_HueLights').append( '<style>' + 
			'.btnhuecheckbox { margin:0px;width:40px; } \n' + 
			'.btnhuecheckbox:after { margin:0px !important; left: 25% !important; top: 25% !important;} ' + 
			'.ui-collapsible-heading, .ui-collapsible-heading-collapsed { width: 100% !important; } ' + 
			'.ui-collapsible-heading-toggle { width: 85% !important; } ' + 
			'.btnhuelightchecked { background-color: #005000 !important; } ' +
			'</style>' ); 
			
			// Update UI for Lights! 
			$('#smart_HueLights').append( '<div data-role="collapsible-set">' ); 
			for(var num in hLights) {
				console.log('Lights obj: ', hLights[num]);
				gHLH(hLights[num], '#smart_HueLights');
			}
			$('#smart_HueLights').append( '</div>' ); 
			
			// Enable/disable Buttons
			$('#smart_ConnectHueBridge').addClass("ui-disabled");
			$('#smart_DisconnectHueBridge').removeClass("ui-disabled");
			$('#smart_RefreshHueDevices').removeClass("ui-disabled");
			
			// Hide ConnectInfo
			$('#smart_hue_connectinfo').hide();
			
			var checked = $("#smart_hue_support").prop("checked"); 
			console.log("TRIGGER " + checked);
			$("#smart_hue_support").prop("checked", checked).trigger("change");
		}, reason => { 
			console.log('hGL ERROR');
		});
	}					
}

function gHLHC(newLight, szIdToAppend) {
	var lID = newLight.uniqueid.replace(/\:|\-/g, ""); 
	var smart_huelight = 'smart_huelight_' + szLightID; 
	var sED = 'smart_enabledevice_' + lID;

	var h = '<div data-role="collapsible" id="' + sHL + '">';
	h += '		<h3 id="' + sHL + '_title">' + newLight.productname + ' ' + newLight.name + (window.D ? ' (ID=' + newLight.uniqueid + ')' : ' ') + '</h3>';
	h += '		<div class="ui-field-contain">';
	h += '			<label for="' + sED + '">Include this device</label>';
	h += '			<input type="checkbox" name="' + sED + '" id="' + sED + '">';
	h += '		</div>';
	h += `		

					<h3 class="ui-bar ui-bar-b ui-corner-all lH">Sector settings:</h3>

					<div id="${sHL}_grid" style="width:248px;height:168px;padding:2px;margin:auto" class="ui-corner-all ui-shadow bTV">								
						<div style="width:240px;height:160px;margin:auto" class="ui-grid-d ui-responsive ui-corner-all bC">
							<div class="ui-grid-d ui-responsive">
				`;
//ui-grid-c = 4 cols
	h += `						
								<div class="ui-block-a" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_0" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_0" id="${sHL}_0" checked data-iconpos="notext">
								</div>								
								<div class="ui-block-b" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_1" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_1" id="${sHL}_1" checked data-iconpos="notext">
								</div>								
								<div class="ui-block-c" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_2" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_2" id="${sHL}_2" checked data-iconpos="notext">
								</div>								
								<div class="ui-block-d" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_3" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_3" id="${sHL}_3" checked data-iconpos="notext">
								</div>								
								<div class="ui-block-e" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_4" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_4" id="${sHL}_4" checked data-iconpos="notext">
								</div>								

								<div class="ui-block-a" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_5" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_5" id="${sHL}_5" checked data-iconpos="notext">
								</div>
								<div class="ui-block-b" style="width:48px;height:55px;float:left;">
								</div>								
								<div class="ui-block-c" style="width:48px;height:55px;float:left;">
								</div>								
								<div class="ui-block-d" style="width:48px;height:55px;float:left;">
								</div>								
								<div class="ui-block-e" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_6" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_6" id="${sHL}_6" checked data-iconpos="notext">
								</div>								

								<div class="ui-block-a" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_7" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_7" id="${sHL}_7" checked data-iconpos="notext">
								</div>								
								<div class="ui-block-b" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_8" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_8" id="${sHL}_8" checked data-iconpos="notext">
								</div>								
								<div class="ui-block-c" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_9" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_9" id="${sHL}_9" checked data-iconpos="notext">
								</div>								
								<div class="ui-block-d" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_10" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_10" id="${sHL}_10" checked data-iconpos="notext">
								</div>								
								<div class="ui-block-e" style="width:48px;height:55px;float:left;">
									<label for="${sHL}_11" class="btnhuecheckbox ui-icon-minus"></label>
									<input type="checkbox" name="${sHL}_11" id="${sHL}_11" checked data-iconpos="notext">
								</div>								
							</div>
						</div>
					</div>


					<h3 class="ui-bar ui-bar-b ui-corner-all lH">Color Correction:</h3>
					<div class="ui-field-contain">
						<label for="slider_${sHL}_colBrightness">Max Brightness:</label>
						<input type="range" name="slider_${sHL}_colBrightness" id="slider_${sHL}_colBrightness" value="255" min="0" max="255" data-highlight="true" data-popup-enabled="true" style="background:#0000000" data-theme="b">
					</div>
					<div class="ui-field-contain">
						<label for="slider_${sHL}_colCorRed">Red:</label>
						<input type="range" name="slider_${sHL}_colCorRed" id="slider_${sHL}_colCorRed" value="100" min="1" max="200" data-highlight="true" data-popup-enabled="true" style="background:#FF0000" data-theme="b">
					</div>
					<div class="ui-field-contain">
						<label for="slider_${sHL}_colCorGreen">Green:</label>
						<input type="range" name="slider_${sHL}_colCorGreen" id="slider_${sHL}_colCorGreen" value="100" min="1" max="200" data-highlight="true" data-popup-enabled="true" style="background:#00A000" data-theme="b">
					</div>
					<div class="ui-field-contain">
						<label for="slider_${sHL}_colCorBlue">Blue:</label>
						<input type="range" name="slider_${sHL}_colCorBlue" id="slider_${sHL}_colCorBlue" value="100" min="1" max="200" data-highlight="true" data-popup-enabled="true" style="background:#0000FF" data-theme="b">
					</div>
	`;
	h += '';
	h += '	  </div>';

	// Append UI for Light
	$(szIdToAppend).append( h ).collapsibleset('refresh');
	// Init JQuery by calling create on parent element
	$("#" + sHL).trigger('create');

	// --- Init UI fields from Controller
	if (typeof window.hV[newLight.uniqueid] !== 'undefined') {
		console.log ("Config for light exists: ", newLight.uniqueid); 
		if (window.hV[newLight.uniqueid].enabled == true) {
			$("#" + sED).prop('checked', true).checkboxradio('refresh');
			$("#" + sHL + "_title").find("a").addClass('btnhuelightchecked');
		} else {
			$("#" + sHL + "_title").find("a").removeClass('btnhuelightchecked');
		}

		for	(var i = 0; i < hZ; i++) {
			if (typeof window.hV[newLight.uniqueid].zones[i] !== 'undefined') {
				$("#" + sHL + "_" + i).prop('checked', window.hV[newLight.uniqueid].zones[i]).checkboxradio('refresh');
			}
		}	
		if (typeof window.hV[newLight.uniqueid].colbrightness !== 'undefined') {
			$("#slider_" + sHL +"_colBrightness").val(window.hV[newLight.uniqueid].colbrightness, true).slider("refresh");
		}
		if (typeof window.hV[newLight.uniqueid].colcorred !== 'undefined') {
			$("#slider_" + sHL +"_colCorRed").val(window.hV[newLight.uniqueid].colcorred, true).slider("refresh");
		}
		if (typeof window.hV[newLight.uniqueid].colcorgreen !== 'undefined') {
			$("#slider_" + sHL +"_colCorGreen").val(window.hV[newLight.uniqueid].colcorgreen, true).slider("refresh");
		}
		if (typeof window.hV[newLight.uniqueid].colcorblue !== 'undefined') {
			$("#slider_" + sHL +"_colCorBlue").val(window.hV[newLight.uniqueid].colcorblue, true).slider("refresh");
		}
	}

	// --- Bind Events!
	$( "#" + sED ).on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		if (ischecked == 1) {
			$("#" + sHL + "_title").find("a").addClass('btnhuelightchecked');
		} else {
			$("#" + sHL + "_title").find("a").removeClass('btnhuelightchecked');
		}
		$( "#result" ).load( "setparam?param=sethuelightenable&id=" + newLight.uniqueid + "&value=" + ischecked, hCR );
	});
	
	// Bind Hitboxes
	var h = '<script>';
	for (var i = 0; i < hZ; i++) {
	h += `$( "#${sHL}_${i}" ).on( "change", function( event, ui ) {
				var ischecked = ($(this).prop("checked") ? 1 : 0); 
					$( "#result" ).load( "setparam?param=sethuelightzone&id=${newLight.uniqueid}&zone=${i}&value=" + ischecked, hCR );
				});`;		
	}
	h += '</script>';
	// Append JS to UI
	$(szIdToAppend).append( h ); 

	// Color Correction
	$( "#slider_" + sHL +"_colBrightness" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=sethuelightcolbrightness&id=" + newLight.uniqueid + "&value=" + $(this).val(), hCR );
	});
	$( "#slider_" + sHL +"_colCorRed" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=sethuelightcolcorred&id=" + newLight.uniqueid + "&value=" + $(this).val(), hCR );
	});
	$( "#slider_" + sHL +"_colCorGreen" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=sethuelightcolcorgreen&id=" + newLight.uniqueid + "&value=" + $(this).val(), hCR );
	});
	$( "#slider_" + sHL +"_colCorBlue" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=sethuelightcolcorblue&id=" + newLight.uniqueid + "&value=" + $(this).val(), hCR );
	});
	
	// ---
	// Fix Sliders Scrolling issue
	fSN(); 
}

function gHLH(newLight, idA) {
	var lID = newLight.uniqueid.replace(/\:|\-/g, ""); 
	var smart_huelight = 'smart_huelight_' + szLightID; 
	var sED = 'smart_enabledevice_' + lID;
	
	// Check if Hue Light DIV exists before!
	if ( $( "#" + sHL).length > 0){
		// div exists
		console.log( "#" + sHL + " DIV exists already ");
	} else {
		// Load Settings from controller
		$.getScript( "controllersettings_hue.js?id=" + newLight.uniqueid )
		  .done(function( script, textStatus ) {
			console.log( textStatus );
			gHLHC(newLight, idA); 
		  })
		  .fail(function( jqxhr, settings, exception ) {
			console.log( "Triggered ajaxError handler." );
			$( "#result" ).text( "Triggered ajaxError handler." );
			gHLHC(newLight, idA); 
		});
	}	
}




				
