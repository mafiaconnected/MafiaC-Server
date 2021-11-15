"use strict";

// ----------------------------------------------------------------------------

// Configuration
let nametagFont = null;
let afkStatusFont = null;
let pingFont = null;
let nametagDistance = 50.0;
let nametagWidth = 70;
let nametagColour = [
    [0,0,0],                    		// (Invalid game)
    [255, 255, 255],            		// GTA3
	[255, 255, 255],            		// GTAVC
	[255, 255, 255],            		// GTASA
	[255, 255, 255],            		// GTAUG
	[255, 255, 255],            		// GTAIV
	[255, 255, 255],            		// GTAEFLC
	[255, 255, 255],            		// INVALID
	[255, 255, 255],            		// INVALID
	[255, 255, 255],            		// INVALID
	[255, 255, 255],            		// Mafia
	[255, 255, 255],            		// Mafia II
	[255, 255, 255],            		// Mafia III
	[255, 255, 255],            		// Mafia DE
];

// ----------------------------------------------------------------------------

addEventHandler("OnResourceReady", function(event, resource) {
	if (resource == thisResource) {
		nametagFont = lucasFont.createDefaultFont(12.0, "Roboto", "Light");

		// For custom font (ttf)
		// Remember to add the font file to the resource folder!
		// And add to meta.xml using this: <file src="aurora-bold-condensed.ttf" type="client" />
		// Replace font file path with whatever custom font you want (TTF format only)
		// ---------------------------------------------------
		//let fontStream = openFile("aurora-bold-condensed.ttf");
		//if(fontStream != null) {
		//	nametagFont = lucasFont.createFont(fontStream, 22.0);
		//	fontStream.close();
		//}


	}
});

// ----------------------------------------------------------------------------

function drawNametag(x, y, health, text, alpha) {
	if(nametagFont == null) {
		return false;
	}

	alpha *= 0.75;
	health = Math.max(0.0, Math.min(1.0, health));

	let colour = COLOUR_WHITE;

	y -= 5;

	if(health > 0.0) {
		let hx = x-nametagWidth/2;
		let hy = y-10/2;
		let colour = toColour(Math.floor(255.0*alpha), Math.floor(255.0-(health*255.0)), Math.floor(health*255.0), 0); // Health bar colour (varies, depending on health)
		drawing.drawRectangle(null, [hx+2, hy+2], [(nametagWidth-4)*health, 10-6], colour, colour, colour, colour);
	}

	y -= 20;

    // Nametag
	if(nametagFont != null) {
		let size = nametagFont.measure(text, game.width, 0.5, 0.0, nametagFont.size, false, false);
		nametagFont.render(text, new Vec2(x-size.x/2, y-size.y/2), game.width, 0.5, 0.0, nametagFont.size, colour, false, false, false, false);
	}
}

// ----------------------------------------------------------------------------

function updateNametag(element) {
	if(localPlayer != null) {
		let elementPos = element.position;
		let health = 100; // element.health;

		elementPos.y += 0.9;

		let screenPos = getScreenFromWorldPosition(element.position);
		let distance = localPlayer.position.distance(element.position);
		//console.log(`${screenPos.x} ${screenPos.y}`);

		if(distance < nametagDistance) {
			let colour = COLOUR_WHITE;
			drawNametag(screenPos.x, screenPos.y, health, element.name, 1.0-distance/nametagDistance);
		}
	}
}

// ----------------------------------------------------------------------------

addEventHandler("OnDrawnHUD", function(event) {
	let players = getElementsByType(ELEMENT_PLAYER);
	for(let i in players) {
		if(players[i] != localPlayer) {
			updateNametag(players[i]);
        }
	}
});

// ----------------------------------------------------------------------------