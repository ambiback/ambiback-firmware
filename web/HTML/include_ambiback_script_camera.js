/* Ambiback Camera Javascripts */
var i=0;
var ecr = {
	FRAMESIZE_QQVGA: i++,
	FRAMESIZE_QQVGA2: i++,
	FRAMESIZE_QCIF: i++,
	FRAMESIZE_HQVGA: i++,
	FRAMESIZE_QVGA: i++,
	FRAMESIZE_CIF: i++,
//	FRAMESIZE_VGA: i++,
//	FRAMESIZE_SVGA: i++,
//	FRAMESIZE_XGA: i++,
//	FRAMESIZE_SXGA: i++,
//	FRAMESIZE_UXGA: i++,
//	FRAMESIZE_QXGA: i++,
}

i=0;
var cPerf = {
	fps50: i++,
	fps45: i++,
	fps40: i++,
	fps35: i++,
	fps30: i++,
	fps25: i++,
	fps20: i++,
}
	
var Res = {
	properties: {
		0: {name: "160x120", width: 160, height: 120, value: ecr.FRAMESIZE_QQVGA},
		1: {name: "128x160", width: 128, height: 160, value: ecr.FRAMESIZE_QQVGA2},
		2: {name: "176x144", width: 176, height: 144, value: ecr.FRAMESIZE_QCIF},
		3: {name: "240x176", width: 240, height: 176, value: ecr.FRAMESIZE_HQVGA},
		4: {name: "320x240", width: 320, height: 240, value: ecr.FRAMESIZE_QVGA},
		5: {name: "400x296", width: 400, height: 296, value: ecr.FRAMESIZE_CIF},
//		6: {name: "640x480", width: 640, height: 480, value: ecr.FRAMESIZE_VGA},
//		7: {name: "800x600", width: 800, height: 600, value: ecr.FRAMESIZE_SVGA},
//		8: {name: "1024x768", width: 1024, height: 768, value: ecr.FRAMESIZE_XGA},
//		9: {name: "1280x1024", width: 1280, height: 1024, value: ecr.FRAMESIZE_SXGA},
//		10: {name: "1600x1200", width: 1600, height: 1200, value: ecr.FRAMESIZE_UXGA},
//		11: {name: "2048x1536", width: 2048, height: 1536, value: ecr.FRAMESIZE_QXGA},
	}
}

var cFreq = {
	properties: {
		0: {name: "10 Mhz", value: 0 },
		1: {name: "12 Mhz", value: 1 },
		2: {name: "14 Mhz", value: 2 },
		3: {name: "16 Mhz", value: 3 },
		4: {name: "18 Mhz", value: 4 },
		5: {name: "20 Mhz", value: 5 },
		6: {name: "22 Mhz", value: 6 },
		7: {name: "24 Mhz", value: 7 },
		8: {name: "25 Mhz", value: 8 },
		9: {name: "26 Mhz", value: 9 },
		10: {name: "27 Mhz", value: 10 },
	}
}

var cFlick= {
	properties: {
		0: {name: "Auto", value: 0 },
		1: {name: "50 Hz", value: 1 },
		2: {name: "60 Hz", value: 2 },
	}
}

i=1;
var ecp = {
	autodefault: i++,
	autosunny: i++,
	autocloudy : i++,
	autocolorful: i++,
	autooffice: i++,
	manualdefault: i++,
	custom: 999,
}

i=1;
var eep = {
	adaptivelens: i++,
	adaptive: i++,
	normal: i++,
	bright: i++,
	verybright: i++,
	auto: i++,
	adaptiveraw: i++,
	adaptivelensraw: i++,
	custom: 999,
}

i=1;
var ecip = {
	veryminimal: i++,
	minimal: i++,
	normal: i++,
	large: i++,
	verylarge: i++,
	custom: 999,
}

i=0;
var cSmooth = {
	disabled: i++,
	low: i++,
	medium: i++,
	high: i++,
	custom: 255,
}

// Default camfile
var cF = "cam.jpg";
var cR = 1; 
var cPts = {
	Point1: {x:0,y:0},
	Point2: {x:100,y:0},
	Point3: {x:0,y:100},
	Point4: {x:100,y:100},
}
var cPS = 20;
var nImg = "";
var nImgGrid = "";

function gWRO(parentWindow, elem) {
	var offset = {
	left : 0,
	top : 0
	};
	// relative to the target field's document
	offset.left = elem.getBoundingClientRect().left;
	offset.top = elem.getBoundingClientRect().top;
	// now we will calculate according to the current document, this current
	// document might be same as the document of target field or it may be
	// parent of the document of the target field
	var childWindow = elem.document.frames.window;
	while (childWindow != parentWindow) {
	offset.left = offset.left + childWindow.frameElement.getBoundingClientRect().left;
	offset.top = offset.top + childWindow.frameElement.getBoundingClientRect().top;
	childWindow = childWindow.parent;
	}
	return offset;
};

function uBI(image, includeGrid, updateGrid) {
	var dateNow = new Date();
	nImg = image+"?"+dateNow.getTime();
	if (updateGrid) { 
		nImgGrid = "cam-grid.bmp?"+dateNow.getTime();
	}

	if (image.search(".mjpeg") == -1 ) {
		$( "#camera_view_img" ).attr('src', 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=');
		if (includeGrid) {
			$( "#camera_view_img" ).css({"background-image":'url(' + nImg + '), url(' + nImgGrid + ')'}); 
		} else {
			$( "#camera_view_img" ).attr('src', nImg);
		}
	} else {
		$( "#camera_view_img" ).attr('src', nImg);
	}
//	$( "#camera_view" ).css({"background-image":'url(' + nImg + ')'});
//	$( "#camera_view_img" ).attr('src', nImg);
	
	$( "#result" ).text("#updated #camera_view_img image to moved to: " + nImg );
}

function hCDS(element, pointnum, ui, posoffsetx, posoffsety) {
	$( "#result" ).text("#camera_point_" + pointnum + " moved to: " + JSON.stringify(ui.position) );
	var iWidth = $( "#camera_view_img" ).width(); 
	var iHeight = $( "#camera_view_img" ).height(); 
	var iLeftOffset = Math.round($( "#camera_view_img" ).position().left);
	var iTopOffset = Math.round($( "#camera_view_img" ).position().top);
	var iLeft =		(Math.round(ui.position.left) + posoffsetx); 
	var iTop =		(Math.round(ui.position.top) + posoffsety); 
//	console.log("left: " + iLeft + "offset: " +  ui.offset.left);
//	console.log("top: " + iTop + "offset: " +  ui.offset.top);
	iLeft -= iLeftOffset; 
	iTop -= iTopOffset; 
//	console.log("left: " + iLeft + "offset: " +  ui.offset.left);
//	console.log("top: " + iTop + "offset: " +  ui.offset.top);
	
	// Calc relativ offsets
	var offsetx = Math.round(iLeft / (iWidth / 100));
	var offsety = Math.round(iTop / (iHeight / 100));
	console.log("Calced: W" + iWidth+ "/R" + offsetx + "/PX" + iLeft + "(" + Math.round(ui.position.left) + ") H" + iHeight + "/R" + offsety + "/PX" + iTop + "(" + Math.round(ui.position.top) + ")"); 

		if (element == "#camera_point_1") {
			cPts.Point1.x = offsetx;
			cPts.Point1.y = offsety;
		} else if (element == "#camera_point_2") {
			cPts.Point2.x = offsetx;
			cPts.Point2.y = offsety;
		} else if (element == "#camera_point_3") {
			cPts.Point3.x = offsetx;
			cPts.Point3.y = offsety;
		} else if (element == "#camera_point_4") {
			cPts.Point4.x = offsetx;
			cPts.Point4.y = offsety;
		}
	// Send point to controller
	$( "#result" ).load( "setcamerapoint?point=" + pointnum + "&left=" +  offsetx + "&top=" + offsety, hCR );
	
	
}

function uCP() {
	sCP("#camera_point_1", cPts.Point1.x, cPts.Point1.y, 0, 0);
	sCP("#camera_point_2", cPts.Point2.x, cPts.Point2.y, -cPS, 0);
	sCP("#camera_point_3", cPts.Point3.x, cPts.Point3.y, 0, -cPS);
	sCP("#camera_point_4", cPts.Point4.x, cPts.Point4.y, -cPS, -cPS);
}

function sCP(element, offsetx, offsety, posoffsetx, posoffsety) {
	var iWidth = $( "#camera_view_img" ).width(); 
	var iHeight = $( "#camera_view_img" ).height(); 
	var iLeftOffset = Math.round($( "#camera_view_img" ).position().left);
	var iTopOffset = Math.round($( "#camera_view_img" ).position().top);
	
	if (offsetx >= 0 && offsetx <= 100 && 
		offsety >= 0 && offsety <= 100)
	{
		if (element == "#camera_point_1") {
			cPts.Point1.x = offsetx;
			cPts.Point1.y = offsety;
		} else if (element == "#camera_point_2") {
			cPts.Point2.x = offsetx;
			cPts.Point2.y = offsety;
		} else if (element == "#camera_point_3") {
			cPts.Point3.x = offsetx;
			cPts.Point3.y = offsety;
		} else if (element == "#camera_point_4") {
			cPts.Point4.x = offsetx;
			cPts.Point4.y = offsety;
		}
		console.log(element + ": x" + offsetx + "/y" + offsety + ": ox" + iLeftOffset + "/oy" + iTopOffset); 
	}

	// Calc real offsets from relativ values
	var iLeft = Math.round((iWidth / 100) * offsetx ) + posoffsetx + iLeftOffset;
	var iTop = Math.round((iHeight / 100) * offsety) + posoffsety + iTopOffset;
	console.log("Calced: R" + offsetx + "/PX" + iLeft + " R" + offsety + "/PX" + iTop); 
	// Set element
	$(element).css({ "left": iLeft + "px", "top": iTop + "px" });
}

//$(document).on( "pagecreate", function( event ) {
//});


function iCT() {
	// Init the fucking TAB!
	$("#tabs").trigger('create');
	
	// --- Camera Settings
	// Bind dots to camera view
	$("#camera_point_1" ).draggable({containment: $("#camera_view_img")});
	$("#camera_point_2" ).draggable({containment: $("#camera_view_img")});
	$("#camera_point_3" ).draggable({containment: $("#camera_view_img")});
	$("#camera_point_4" ).draggable({containment: $("#camera_view_img")});
	/* NOT NEEDED ANYMORE - DONE BY controllersettings.js 
	// Move camera points to their default location
	$("#camera_point_1").css({ left: '0px' });
	$("#camera_point_2").css({ left: '310px' });
	$("#camera_point_3").css({ top: '230px' });
	$("#camera_point_4").css({ top: '230px', left: '310px' });
	*/
	// Handle camera point clicks
	$("#camera_point_1").on( "dragstop", function( event, ui ) { hCDS("#camera_point_1", 1, ui, 0, 0); });
	$("#camera_point_2").on( "dragstop", function( event, ui ) { hCDS("#camera_point_2", 2, ui, cPS, 0); });
	$("#camera_point_3").on( "dragstop", function( event, ui ) { hCDS("#camera_point_3", 3, ui, 0, cPS); });
	$("#camera_point_4").on( "dragstop", function( event, ui ) { hCDS("#camera_point_4", 4, ui, cPS, cPS); });

	sCP("#camera_point_1", 0, 0, 0, 0);
	sCP("#camera_point_2", 100, 0, -cPS, 0);
	sCP("#camera_point_3", 0, 100, 0, -cPS);
	sCP("#camera_point_4", 100, 100, -cPS, -cPS);

	// --- 
	
	// Init BrightnessPresets
	$("#camera_Performance").append(new Option("--select--", "select"));
	$("#camera_Performance").val("select");
	$("#camera_Performance").selectmenu("refresh");
	$("#camera_Performance").append(new Option("50fps", cPerf.fps50));
	$("#camera_Performance").append(new Option("40fps", cPerf.fps45));
	$("#camera_Performance").append(new Option("40fps", cPerf.fps40));
	$("#camera_Performance").append(new Option("35fps", cPerf.fps35));
	$("#camera_Performance").append(new Option("30fps", cPerf.fps30));
	$("#camera_Performance").append(new Option("25fps", cPerf.fps25));
	$("#camera_Performance").append(new Option("20fps", cPerf.fps20));
	$("#camera_Performance").on( "change", function( event, ui ) {
		console.log($(this).val());

		// Store preset number
		$( "#result" ).load( "setparam?param=setcamperformance&value=" + $(this).val(), hCR );
	});
	
	// Init resolutions 
	$("#camera_resolution").append(new Option("--select--", "select"));
	$('#camera_resolution').val("select");
	$('#camera_resolution').selectmenu("refresh");
	// Fill with values
	for (const key of Object.keys(Res.properties)) {
		var newOpt = new Option(key, key);
		$(newOpt).html(Res.properties[key].name);
		$("#camera_resolution").append(newOpt);
	}
	// --- Input fields
	$( "#camera_resolution" ).on( "change", function( event, ui ) {
		// Save to cR
		cR = $(this).val();
		$( "#result" ).load( "setparam?param=setcamres&value=" + Resolutions.properties[cR].value, hCR );
		uBI(cF, true, true);
		
		// Update css sizes
		var newWidth = (Resolutions.properties[cR].width > 320 ? Resolutions.properties[cR].width : 320); 
		var newheight  = (Resolutions.properties[cR].height > 240 ? Resolutions.properties[cR].height: 240); 
		
		// $( "#camera_view" ).css({"width": newWidth});
		// $( "#camera_view" ).css({"height": newheight});
		$( "#camera_view_img" ).css({"width": newWidth});
		$( "#camera_view_img" ).css({"height": newheight});

		// Update Camera Points!
		uCP(); 
	});

	// Init Cam Frequences 
	$("#camera_camFreq").append(new Option("--select--", "select"));
	$('#camera_camFreq').val("select");
	$('#camera_camFreq').selectmenu("refresh");
	// Fill with values
	for (const key of Object.keys(cFreq.properties)) {
		var newOpt = new Option(key, key);
		$(newOpt).html(cFreq.properties[key].name);
		$("#camera_camFreq").append(newOpt);
	}
	$( "#camera_camFreq" ).on( "change", function( event, ui ) {
		// Save to cFr
		cFr = $(this).val();
		$( "#result" ).load( "setparam?param=setcamfreq&value=" + cFr, hCRAR );
	});
	
	// Init FB Count 
	$("#camera_camFbcount").append(new Option("--select--", "select"));
	$('#camera_camFbcount').val("select");
	$('#camera_camFbcount').selectmenu("refresh");
	// Init from window variables
	if (typeof window.ab.cmfc  !== 'undefined') {
		for	(var i = 1; i <= window.ab.cmfc; i++) {
			$("#camera_camFbcount").append(new Option(i, i));
		}	
	}
	$( "#camera_camFbcount" ).on( "change", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcambufcount&value=" + $(this).val(), hCRAndReboot );
	});
	
	// Init Flickering Options 
	$("#camera_camFlicker").append(new Option("--select--", "select"));
	$('#camera_camFlicker').val("select");
	$('#camera_camFlicker').selectmenu("refresh");
	// Fill with values
	for (const key of Object.keys(cFlick.properties)) {
		var newOpt = new Option(key, key);
		$(newOpt).html(cFlick.properties[key].name);
		$("#camera_camFlicker").append(newOpt);
	}
	$( "#camera_camFlicker" ).on( "change", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamflicker&value=" + $(this).val(), hCR );
	});
	
	// Camera Buttons
	$("#camera_img_jpg").on('change', function( event, ui ){ 
		cF = $(this).val();
		uBI("/images/ajax-loader.gif", false, false);
		uBI(cF, true, true);
		$("#camera_img_refresh").removeClass("ui-disabled");
		$("#camera_img_stop").addClass("ui-disabled");
	});
	$("#camera_img_mjpeg").on('change', function( event, ui ){ 
		cF = $(this).val();
		uBI(cF, false, false); 
		$("#camera_img_refresh").addClass("ui-disabled");
		$("#camera_img_stop").removeClass("ui-disabled");
	});
	$("#camera_img_refresh").on('click', function(){ 
		uBI("/images/ajax-loader.gif", false, false);
		uBI(cF, true, false); 
	});
	$("#camera_img_stop").on('click', function(){ 
		$('#camera_img_mjpeg').prop('checked', false).checkboxradio("refresh");
		$('#camera_img_jpg').prop('checked', true).checkboxradio("refresh");

	
		$("#camera_img_refresh").removeClass("ui-disabled");
		$("#camera_img_stop").addClass("ui-disabled");
		// Trigger Image Change
		$('#camera_img_jpg').trigger('change');
	});
	
	// Default disabled
	$("#camera_img_stop").addClass("ui-disabled");

	// Color Correction
	$( "#slider_camColorRed" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamcolorred&value=" + $(this).val(), hCR );
	});
	$( "#slider_camColorGreen" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamcolorgreen&value=" + $(this).val(), hCR );
	});
	$( "#slider_camColorBlue" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamcolorblue&value=" + $(this).val(), hCR );
	});
	$( "#slider_camActivationThreshold" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamactivationthreshold&value=" + $(this).val(), hCR );
	});
	$( "#slider_camMinPixelAmount" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamminpixelamount&value=" + $(this).val(), hCR );
	});
	$( "#slider_camsBlackthresholdSmooth" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamblackthresholdsmooth&value=" + $(this).val(), hCR );
	});
	$( "#slider_camBlackRemoveTreshold" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamblackremovethreshold&value=" + $(this).val(), hCR );
	});
	
	// Camera Options
	$( "#slider_camContrast" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamcontrast&value=" + $(this).val(), hCR );
	});
	$( "#slider_camBrightness" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcambrightness&value=" + $(this).val(), hCR );
	});
	$( "#slider_camSaturation" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamsaturation&value=" + $(this).val(), hCR );
	});
	$( "#slider_camAeLevels" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamaelevels&value=" + $(this).val(), hCR );
	});
	$( "#slider_camAgcGain" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamagcgain&value=" + $(this).val(), hCR );
	});
	$( "#slider_camQuality" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamquality&value=" + $(this).val(), hCR );
	});
	$( "#slider_camAecValue" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamaecvalue&value=" + $(this).val(), hCR );
	});
	$( "#slider_camAecMinValue" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamaecminvalue&value=" + $(this).val(), hCR );
	});
	$( "#slider_camAecMaxValue" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamaecmaxvalue&value=" + $(this).val(), hCR );
	});
	$( "#slider_camWhiteActThreshold" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamwhiteactthreshold&value=" + $(this).val(), hCR );
	});
	$( "#slider_camAdaptiveChangePercent" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamadaptivechangepercent&value=" + $(this).val(), hCR );
	});
	$( "#slider_camWhiteMinimumAmount" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamwhiteminimumamount&value=" + $(this).val(), hCR );
	});
	$( "#slider_camBeforeWhiteActivationTreshold" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcambeforewhiteactthreshold&value=" + $(this).val(), hCR );
	});
	$( "#slider_camAdaptiveChangeDelay" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamadaptivechangedelay&value=" + $(this).val(), hCR );
	});

	$( "#slider_camAdaptiveGainDown" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamadaptivegaindown&value=" + $(this).val(), hCR );
	});
	$( "#slider_camAdaptiveGainUp" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamadaptivegainup&value=" + $(this).val(), hCR );
	});
	$( "#slider_camAdaptiveGainMax" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamadaptivegainmax&value=" + $(this).val(), hCR );
	});
	
	$( "#slider_camDetailLevel" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamdetaillevel&value=" + $(this).val(), hCR );
	});
	$( "#slider_camInnerHori" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcaminnerhori&value=" + $(this).val(), hCR );
	});
	$( "#slider_camInnerVert" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcaminnervert&value=" + $(this).val(), hCR );
	});
	
	// Programatic Options
	$( "#slider_camColorBoostMax" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamcolorboostmax&value=" + $(this).val(), hCR );
	});

	// Init CamMode
	$("#camera_CamMode").append(new Option("--select--", "select"));
	$("#camera_CamMode").val("select");
	$("#camera_CamMode").selectmenu("refresh");
	$("#camera_CamMode").append(new Option("Single Color", 0));
	$("#camera_CamMode").append(new Option("Full Lightbar", 1));
	$("#camera_CamMode").append(new Option("Detailed Colors", 2));
	$("#camera_CamMode").on( "change", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcammode&value=" + $(this).val(), hCR );
		if ($(this).val() == 2) {
			$("#camera_CamMode_custom").show();
		} else {
			$("#camera_CamMode_custom").hide();
		}
	});
	
	// Init CamSmooth
	$("#camera_CamSmooth").append(new Option("Disabled", cSmooth.disabled));
	$("#camera_CamSmooth").val(0);
	$("#camera_CamSmooth").selectmenu("refresh");
	$("#camera_CamSmooth").append(new Option("Low Smoothing", cSmooth.low));
	$("#camera_CamSmooth").append(new Option("Medium Smoothing", cSmooth.medium));
	$("#camera_CamSmooth").append(new Option("High Smoothing", cSmooth.high));
	$("#camera_CamSmooth").append(new Option("Custom Settings", cSmooth.custom));
	$("#camera_CamSmooth").on( "change", function( event, ui ) {
		console.log($(this).val());

		// Store preset number
		$( "#result" ).load( "setparam?param=setcSmooth&value=" + $(this).val(), hCR );

		/// Show hide custom settings
		if ($(this).val() == cSmooth.custom) {
			// Custom Mode, show Settings
			$("#camera_CamSmooth_custom").show();
		} else {
			// Hide Custom properties
			$("#camera_CamSmooth_custom").hide();
		}
	});
	$("#camera_CamSmoothEnable").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcSmoothenable&value=" + ischecked, hCR );
	});
	$( "#slider_CamSmoothMaxFrames" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcSmoothmaxframes&value=" + $(this).val(), hCR );
	});
	$( "#slider_CamSmoothMaxDiff" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcSmoothmaxdiff&value=" + $(this).val(), hCR );
	});

	// Checkboxes
	$("#camera_Gaincontrol").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0);
		if (!ischecked) { $('#slider_camAgcGain').slider('enable'); } else  { $('#slider_camAgcGain').slider('disable'); }
		$( "#result" ).load( "setparam?param=setcamagaincontrol&value=" + ischecked, hCR );
	});
	$("#camera_Whitebalance").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamawhitebalance&value=" + ischecked, hCR );
	});
	$("#camera_Colorbar").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamcolorbar&value=" + ischecked, hCR );
	});
	$("#camera_aec").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0);
//		if (!ischecked) { $('#slider_camAecValue').slider('enable'); } else  { $('#slider_camAecValue').slider('disable'); }
//		if (ischecked) { $('#camera_Aec2').checkboxradio('enable'); } else  { $('#camera_Aec2').checkboxradio('disable'); }
		$( "#result" ).load( "setparam?param=setcamaec&value=" + ischecked, hCR );
	});
	$("#camera_Aec2").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0);
		$( "#result" ).load( "setparam?param=setcamaec2&value=" + ischecked, hCR );
	});
	
	$("#camera_adptexp").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0);
		$( "#result" ).load( "setparam?param=setcamadptexp&value=" + ischecked, hCR );
	});
	$("#camera_adptgain").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0);
		$( "#result" ).load( "setparam?param=setcamadptgain&value=" + ischecked, hCR );
	});
	$("#camera_AwbGainControl").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		if (ischecked) { $('#camera_WbMode').selectmenu('enable'); } else  { $('#camera_WbMode').selectmenu('disable'); }
		$( "#result" ).load( "setparam?param=setcamawbgain&value=" + ischecked, hCR );
	});
	$("#camera_HMirror").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamhmirror&value=" + ischecked, hCR );
	});
	$("#camera_VFlip").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamvflip&value=" + ischecked, hCR );
	});
	$("#camera_DetailSmooth").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamdetailsmooth&value=" + ischecked, hCR );
	});
	$("#camera_HighQuality").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamhighquality&value=" + ischecked, hCR );
	});
	$("#camera_bpc").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcambpc&value=" + ischecked, hCR );
	});
	$("#camera_wpc").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamwpc&value=" + ischecked, hCR );
	});
	$("#camera_lenc").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamlenc&value=" + ischecked, hCR );
	});
	$("#camera_denoise").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamdenoise&value=" + ischecked, hCR );
	});
	$("#camera_rawgma").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamrawgma&value=" + ischecked, hCR );
	});
	$("#camera_dcw").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamdcw&value=" + ischecked, hCR );
	});
	$("#camera_overclock").on( "change", function( event, ui ) {
		var ischecked = ($(this).prop("checked") ? 1 : 0); 
		$( "#result" ).load( "setparam?param=setcamoverclock&value=" + ischecked, hCR );
	});
	
	// Init CamPresets
	$("#camera_CamPresets").append(new Option("--select--", "select"));
	$("#camera_CamPresets").val("select");
	$("#camera_CamPresets").selectmenu("refresh");
	
	$("#camera_CamPresets").append(new Option("Default Whitebalance", ecp.autodefault));
	$("#camera_CamPresets").append(new Option("Cloudy Whitebalance", ecp.autocloudy));
	$("#camera_CamPresets").append(new Option("Sunny Whitebalance", ecp.autosunny));
	$("#camera_CamPresets").append(new Option("Sunny Colorful Whitebalance", ecp.autocolorful));
	$("#camera_CamPresets").append(new Option("Office Whitebalance", ecp.autooffice));
	$("#camera_CamPresets").append(new Option("Manual Whitebalance", ecp.manualdefault));
	
	$("#camera_CamPresets").append(new Option("Custom Settings", ecp.custom));

	$("#camera_CamPresets").on( "change", function( event, ui ) {
		console.log($(this).val());
		// Store preset number
		$( "#result" ).load( "setpreset?param=setcampreset&value=" + $(this).val(), hCRP );
		
		// Show hide custom settings
		if ($(this).val() == ecp.custom) {
			// Custom Mode, show Settings
			$("#cam_whitebalance_custom").show();
		} else {
			// Hide Custom properties
			$("#cam_whitebalance_custom").hide();
		}
		
	});
	
	// Reset values to DEFAULT
	$("#camera_reset_default").on('click', function(){
		if (typeof window.CamWbMode !== 'undefined') 		$("#camera_WbMode").val(window.CamWbMode).selectmenu("refresh");
		if (typeof window.CamColCorRed !== 'undefined')		$("#slider_camColorRed").val(window.CamColCorRed).slider("refresh");
		if (typeof window.CamColCorGreen !== 'undefined')	$("#slider_camColorGreen").val(window.CamColCorGreen).slider("refresh");
		if (typeof window.CamColCorBlue !== 'undefined')	$("#slider_camColorBlue").val(window.CamColCorBlue).slider("refresh");
		if (typeof window.CamContrast !== 'undefined')		$("#slider_camContrast").val(window.CamContrast).slider("refresh");
		if (typeof window.CamBrightness !== 'undefined')	$("#slider_camBrightness").val(window.CamBrightness).slider("refresh");
		if (typeof window.CamSaturation !== 'undefined')	$("#slider_camSaturation").val(window.CamSaturation).slider("refresh");
		if (typeof window.CamAwb !== 'undefined')			{ $("#camera_Whitebalance").prop("checked",(window.CamAwb == 1 ? true : false)).checkboxradio("refresh"); $('#camera_Whitebalance').trigger('change'); }
		if (typeof window.CamAwbGain !== 'undefined')		{ $("#camera_AwbGainControl").prop("checked",(window.CamAwbGain == 1 ? true : false)).checkboxradio("refresh"); $('#camera_AwbGainControl').trigger('change'); }
		if (typeof window.CamExposureMode !== 'undefined')	{ $("#cam_exposure_Presets").val(window.CamExposureMode).selectmenu("refresh"); }
	});
	
	// Init Special Effect 
	$("#camera_SpecialEffect").append(new Option("--select--", "select"));
	$('#camera_SpecialEffect').val("select");
	$('#camera_SpecialEffect').selectmenu("refresh");
	$("#camera_SpecialEffect").append(new Option("no effect", 0));
	$("#camera_SpecialEffect").append(new Option("negative", 1));
	$("#camera_SpecialEffect").append(new Option("black and white", 2));
	$("#camera_SpecialEffect").append(new Option("reddish", 3));
	$("#camera_SpecialEffect").append(new Option("greenish", 4));
	$("#camera_SpecialEffect").append(new Option("blue", 5));
	$("#camera_SpecialEffect").append(new Option("retro", 6));
	$("#camera_SpecialEffect").on( "change", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamspecialeffect&value=" + $(this).val(), hCR );
	});
	// Init WB Mode
	$("#camera_WbMode").append(new Option("--select--", "select"));
	$("#camera_WbMode").val("select");
	$("#camera_WbMode").selectmenu("refresh");
	$("#camera_WbMode").append(new Option("default", 0));
	$("#camera_WbMode").append(new Option("sunny", 1));
	$("#camera_WbMode").append(new Option("cloudy", 2));
	$("#camera_WbMode").append(new Option("office", 3));
	$("#camera_WbMode").append(new Option("home", 4));
	$("#camera_WbMode").on( "change", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamwbmode&value=" + $(this).val(), hCR );
	});
	// Init GainceilingSensor
	$("#camera_GainceilingSensor").append(new Option("--select--", "select"));
	$('#camera_GainceilingSensor').val("select");
	$('#camera_GainceilingSensor').selectmenu("refresh");
	$("#camera_GainceilingSensor").append(new Option("Gainceiling x2", 0));
	$("#camera_GainceilingSensor").append(new Option("Gainceiling x4", 1));
	$("#camera_GainceilingSensor").append(new Option("Gainceiling x8", 2));
	$("#camera_GainceilingSensor").append(new Option("Gainceiling x16", 3));
	$("#camera_GainceilingSensor").append(new Option("Gainceiling x32", 4));
	$("#camera_GainceilingSensor").append(new Option("Gainceiling x64", 5));
	$("#camera_GainceilingSensor").append(new Option("Gainceiling x128", 6));
	$("#camera_GainceilingSensor").on( "change", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcamagainceilingsensor&value=" + $(this).val(), hCR );
	});

	// --- Color Enhancement Options 
	// Init ColorPresets
	$("#cam_color_Presets").append(new Option("--select--", "select"));
	$("#cam_color_Presets").val("select");
	$("#cam_color_Presets").selectmenu("refresh");
	$("#cam_color_Presets").append(new Option("Normal", ecp.normal));
	$("#cam_color_Presets").append(new Option("Colorful", ecp.colorful));
	$("#cam_color_Presets").append(new Option("Extra Colorful", ecp.extracolorful));
	$("#cam_color_Presets").append(new Option("Extreme Colorful", ecp.extremecolorful));
	$("#cam_color_Presets").append(new Option("Custom Settings", ecp.custom));
	$("#cam_color_Presets").on( "change", function( event, ui ) {
		console.log($(this).val());

		// Store preset number
		$( "#result" ).load( "setpreset?param=setcolorpreset&value=" + $(this).val(), hCRP );

		/// Show hide custom settings
		if ($(this).val() == ecp.custom) {
			// Custom Mode, show Settings
			$("#cam_color_enhancement_custom").show();
		} else {
			// Hide Custom properties
			$("#cam_color_enhancement_custom").hide();
		}
	});
	$( "#cam_slider_colorBoost" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcolorboost&value=" + $(this).val(), hCR );
	});
	$( "#cam_slider_colorBoostBalance" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcolorboostbalance&value=" + $(this).val(), hCR );
	});
	$( "#cam_slider_colorBoostMin" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcolorboostmin&value=" + $(this).val(), hCR );
	});
	$( "#cam_slider_colorBoostRed" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcolorboostred&value=" + $(this).val(), hCR );
	});
	$( "#cam_slider_colorBoostGreen" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcolorboostgreen&value=" + $(this).val(), hCR );
	});
	$( "#cam_slider_colorBoostBlue" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcolorboostblue&value=" + $(this).val(), hCR );
	});
	$( "#cam_slider_colorBoostThreshold" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcolorboostthreshold&value=" + $(this).val(), hCR );
	});
	$( "#cam_slider_colorValueGain" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setcolorvaluegain&value=" + $(this).val(), hCR );
	});
	$( "#cam_slider_colorMinimumLuminosity" ).on( "slidestop", function( event, ui ) {
		$( "#result" ).load( "setparam?param=setminimumluminosity&value=" + $(this).val(), hCR );
	});
	// --- 
	
	// --- Exposure Correction Options 
	// Init Exposure Presets
	$("#cam_exposure_Presets").append(new Option("--select--", "select"));
	$("#cam_exposure_Presets").val("select");
	$("#cam_exposure_Presets").selectmenu("refresh");
	$("#cam_exposure_Presets").append(new Option("Adaptive TV Mode 1 (Raw Gamma)", eep.adaptiveraw));
	$("#cam_exposure_Presets").append(new Option("Adaptive TV Mode 2 (Lens Correction)", eep.adaptivelens));
	$("#cam_exposure_Presets").append(new Option("Adaptive TV Mode 3 (Default)", eep.adaptive));
	$("#cam_exposure_Presets").append(new Option("Adaptive TV Mode 4 (Raw Gamma & Lens Correction)", eep.adaptivelensraw));

	$("#cam_exposure_Presets").append(new Option("TEST Auto Sensor Control", eep.auto));
	$("#cam_exposure_Presets").append(new Option("TEST Manual Normal", eep.normal));
	$("#cam_exposure_Presets").append(new Option("TEST Manual Bright", eep.bright));
		$("#cam_exposure_Presets").append(new Option("TEST Manual Very bright", eep.verybright));

	$("#cam_exposure_Presets").append(new Option("Custom Settings", eep.custom));
	
	$("#cam_exposure_Presets").on( "change", function( event, ui ) {
		console.log($(this).val());

		// Store preset number
		$( "#result" ).load( "setpreset?param=setcamexposurepreset&value=" + $(this).val(), hCRP );
	});

	// Init BrightnessPresets
	$("#cam_brightness_Presets").append(new Option("--select--", "select"));
	$("#cam_brightness_Presets").val("select");
	$("#cam_brightness_Presets").selectmenu("refresh");
	$("#cam_brightness_Presets").append(new Option("Low", ebp.low));
	$("#cam_brightness_Presets").append(new Option("Medium", ebp.medium));
	$("#cam_brightness_Presets").append(new Option("Normal", ebp.normal));
	$("#cam_brightness_Presets").append(new Option("High", ebp.high));
	$("#cam_brightness_Presets").append(new Option("Extra High", ebp.extrahigh));
	$("#cam_brightness_Presets").append(new Option("Extreme High", ebp.extremehigh));
	$("#cam_brightness_Presets").append(new Option("Custom Settings", ebp.custom));
	$("#cam_brightness_Presets").on( "change", function( event, ui ) {
		console.log($(this).val());

		// Store preset number
		$( "#result" ).load( "setpreset?param=setbrightnesspreset&value=" + $(this).val(), hCRP );

		/// Show hide custom settings
		if ($(this).val() == ebp.custom) {
			// Custom Mode, show Settings
			$("#cam_brightness_enhancement_custom").show();
		} else {
			// Hide Custom properties
			$("#cam_brightness_enhancement_custom").hide();
		}
	});
	// --- 
	
	// Reset values to DEFAULT
	$("#cam_exposure_reset_default").on('click', function() {
		if (typeof window.CamAec !== 'undefined')			{ 
			$("#camera_aec").prop("checked",(window.CamAec == 1 ? true : false)).checkboxradio("refresh"); 
			$('#camera_aec').trigger('change');
		}
		if (typeof window.CamAec2 !== 'undefined') {
			$("#camera_Aec2").prop("checked", (window.CamAec2 == 1 ? true : false)).checkboxradio("refresh");
			$('#camera_Aec2').trigger('change');
		}
		if (typeof window.CamAelevels !== 'undefined') {
			$("#slider_camAeLevels").val(window.CamAelevels, true).slider("refresh");
			$("#slider_camAeLevels").trigger('slidestop');
		}
		if (typeof window.CamAecValue !== 'undefined') {
			$("#slider_camAecValue").val(window.CamAecValue, true).slider("refresh");
			$("#slider_camAecValue").trigger('slidestop');
		}
		if (typeof window.CamGain !== 'undefined') {
			$("#camera_Gaincontrol").prop("checked",(window.CamGain == 1 ? true : false)).checkboxradio("refresh"); 
			$('#camera_Gaincontrol').trigger('change');
		}
		if (typeof window.CamAgcGain !== 'undefined') {
			$("#slider_camAgcGain").val(window.CamAgcGain, true).slider("refresh");
			$("#slider_camAgcGain").trigger('slidestop');
		}
		if (typeof window.CamRawGMA !== 'undefined') {
			$("#camera_rawgma").prop("checked",(window.CamRawGMA == 1 ? true : false)).checkboxradio("refresh");
			$('#camera_rawgma').trigger('change'); 
		}
		if (typeof window.CamLenc !== 'undefined') {
			$("#camera_lenc").prop("checked",(window.CamLenc == 1 ? true : false)).checkboxradio("refresh");
			$('#camera_lenc').trigger('change'); 
		}
		if (typeof window.CamAecMinValue !== 'undefined') { 
			$("#slider_camAecMinValue").val(window.CamAecMinValue, true).slider("refresh");
			$("#slider_camAecMinValue").trigger('slidestop');
		}
		if (typeof window.CamAecMaxValue !== 'undefined') {
			$("#slider_camAecMaxValue").val(window.CamAecMaxValue, true).slider("refresh");
			$("#slider_camAecMaxValue").trigger('slidestop');
		}
		if (typeof window.CamAdaptiveChangePercent!== 'undefined') {
			$("#slider_camAdaptiveChangePercent").val(window.CamAdaptiveChangePercent, true).slider("refresh");
			$("#slider_camAdaptiveChangePercent").trigger('slidestop');
		}
		if (typeof window.CamAdaptiveChangeDelay !== 'undefined') { 
			$("#slider_camAdaptiveChangeDelay").val(window.CamAdaptiveChangeDelay, true).slider("refresh");
			$("#slider_camAdaptiveChangeDelay").trigger('slidestop');
		}
		if (typeof window.CamWhiteActivationTreshold !== 'undefined') { 
			$("#slider_camWhiteActThreshold").val(window.CamWhiteActivationTreshold, true).slider("refresh");
			$("#slider_camWhiteActThreshold").trigger('slidestop');
		}
		if (typeof window.CamBeforeWhiteActivationTreshold  !== 'undefined') { 
			$("#slider_camBeforeWhiteActivationTreshold").val(window.CamBeforeWhiteActivationTreshold, true).slider("refresh");
			$("#slider_camBeforeWhiteActivationTreshold").trigger('slidestop');
		}
		if (typeof window.CamWhiteMinimumAmount !== 'undefined') { 
			$("#slider_camWhiteMinimumAmount").val(window.CamWhiteMinimumAmount, true).slider("refresh");
			$("#slider_camWhiteMinimumAmount").trigger('slidestop');
		}
		if (typeof window.CamAdaptiveGainDown !== 'undefined') { 
			$("#slider_camAdaptiveGainDown").val(window.CamAdaptiveGainDown, true).slider("refresh");
			$("#slider_camAdaptiveGainDown").trigger('slidestop');
		}
		if (typeof window.CamAdaptiveGainUp !== 'undefined') { 
			$("#slider_camAdaptiveGainUp").val(window.CamAdaptiveGainUp, true).slider("refresh");
			$("#slider_camAdaptiveGainUp").trigger('slidestop');
		}
		if (typeof window.CamAdaptiveGainMax !== 'undefined') { 
			$("#slider_camAdaptiveGainMax").val(window.CamAdaptiveGainMax, true).slider("refresh");
			$("#slider_camAdaptiveGainMax").trigger('slidestop');
		}
		if (typeof window.CamAdaptiveExposurecontrol !== 'undefined') { 	
			$("#camera_adptexp").prop("checked",(window.CamAdaptiveExposurecontrol == 1 ? true : false)).checkboxradio("refresh");
			$('#camera_adptexp').trigger('change');
		}		
		if (typeof window.CamAdaptiveGaincontrol !== 'undefined') { 		
			$("#camera_adptgain").prop("checked",(window.CamAdaptiveGaincontrol == 1 ? true : false)).checkboxradio("refresh");
			$('#camera_adptgain').trigger('change');
		}
		if (typeof window.CamHMirror !== 'undefined') {
			$("#camera_HMirror").prop("checked",(window.CamHMirror == 1 ? true : false)).checkboxradio("refresh");
			$('#camera_HMirror').trigger('change'); 
		}
		if (typeof window.CamVFlip !== 'undefined') {
			$("#camera_VFlip").prop("checked",(window.CamVFlip == 1 ? true : false)).checkboxradio("refresh");
			$('#camera_VFlip').trigger('change'); 
		}
	});

	// --- Cam Inner Usage Options 
	// Init Inner Usage Presets
	$("#cam_inner_Presets").append(new Option("--select--", "select"));
	$("#cam_inner_Presets").val("select");
	$("#cam_inner_Presets").selectmenu("refresh");
	$("#cam_inner_Presets").append(new Option("Very minimal", ecip.veryminimal));
	$("#cam_inner_Presets").append(new Option("Minimal", ecip.minimal));
	$("#cam_inner_Presets").append(new Option("Normal", ecip.normal));
	$("#cam_inner_Presets").append(new Option("Large", ecip.large));
	$("#cam_inner_Presets").append(new Option("Very large", ecip.verylarge));

	$("#cam_inner_Presets").append(new Option("Custom Settings", ecip.custom));
	
	$("#cam_inner_Presets").on( "change", function( event, ui ) {
		console.log($(this).val());

		// Store preset number
		$( "#result" ).load( "setpreset?param=setcaminnerpreset&value=" + $(this).val(), hCRP );

		/// Show hide custom settings
		if ($(this).val() == ecip.custom) {
			// Custom Mode, show Settings
			$("#cam_inner_custom").show();
		} else {
			// Hide Custom properties
			$("#cam_inner_custom").hide();
		}
	});
	// ---

	// Need to catch tabsactivate to correct campoints
	$("#tabs").on( "tabsactivate", function( event, ui ){
		// console.log (ui.newPanel.prop("id") ); 
		if (ui.newPanel.prop("id") == "tabs-camera") {
			// Update Camera Points!
			uCP(); 
		}
	});

	// Hide Custom properties
	$("#camera_CamMode_custom").hide();
	$("#camera_CamSmooth_custom").hide();
	$("#cam_color_enhancement_custom").hide();
	$("#cam_brightness_enhancement_custom").hide();
	$("#cam_inner_custom").hide();
	$("#cam_whitebalance_custom").hide();

	if (typeof window.ab.d !== 'undefined') {
		if (window.ab.d == false) {
			$("#camera_developer").hide();
		} else {
			$("#camera_developer").show();
		}
	} else {
		$("#camera_developer").hide();
	}

	// Set Tab Loaded
	window.tVars['link_camera'] = true;

	// Fix Sliders Scrolling issue
	fSN();
	
	// Hide Loading form!
	hL();
}
// Alias for backward compatibility with HTML
var initCameraTab = iCT;

/*
TESTING
$(document).on( "pagecreate", function( event ) {
	alert("pagecreate");
});

$(document).on( "pageinit", function( event ) {
	alert("pageinit");
});

*/