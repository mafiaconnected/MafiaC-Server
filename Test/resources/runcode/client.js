"use strict";

// ----------------------------------------------------------------------------

let outputColor = toColour(72, 144, 48, 255);

// ----------------------------------------------------------------------------

addCommandHandler("jce", function(command, params) {
    if(!params || params == "") {
        message("Syntax: /sce <code>", findResourceByName("v-utils").exports.getSyntaxMessageColour(gta.game));
        return false;
    }

	let returnVal = eval(params);
	message("JavaScript client code executed: " + params, COLOUR_YELLOW);
});

// ----------------------------------------------------------------------------

addCommandHandler("jcr", function(command, params) {
    if(!params || params == "") {
        message("Syntax: /sce <code>", findResourceByName("v-utils").exports.getSyntaxMessageColour(gta.game));
        return false;
    }

	let returnVal = eval("(" + params + ")")
	message("JavaScript client code executed: " + params, COLOUR_YELLOW);
	message("Returns: " + returnVal, COLOUR_YELLOW);
});

// ===========================================================================

function and(var1, var2) {
	return (var1 && var2);
}

// ===========================================================================

function or(var1, var2) {
	return (var1 || var2);
}

// ===========================================================================

function not(var1) {
	return !var1;
}

// ===========================================================================

function bitand(var1, var2) {
	return var1 & var2;
}

// ===========================================================================

function bitor(var1, var2) {
	return var1 | var2;
}

// ===========================================================================

function bitxor(var1, var2) {
	return var1 ^ var2;
}

// ===========================================================================

function bitnot(var1) {
	return ~var1;
}

// ===========================================================================

function bitl(var1, var2) {
	return var1 << var2;
}

// ===========================================================================

function bitr(var1, var2) {
	return var1 >> var2;
}

// ===========================================================================

function gt(var1, var2) {
	return var1 > var2;
}

// ===========================================================================

function lt(var1, var2) {
	return (var1 < var2);
}

// ===========================================================================

function gteq(var1, var2) {
	return (var1 >= var2);
}

// ===========================================================================

function lteq(var1, var2) {
	return (var1 <= var2);
}

// ===========================================================================

function eq(var1, var2) {
	return (var1 == var2);
}

// ===========================================================================

function modulo(var1, var2) {
	return var1 % var2;
}

// ===========================================================================

function add(...args) {
	return args.reduce((acc, a) => {
		return acc + a;
	}, 0);
}

// ===========================================================================

function sub(...args) {
	return args.reduce((acc, a) => {
		return acc - a;
	}, 0);
}

// ===========================================================================

function mult(...args) {
	return args.reduce((acc, a) => {
		return acc * a;
	}, 0);
}

// ===========================================================================

function div(...args) {
	return args.reduce((acc, a) => {
		return acc / a;
	}, 0);
}

// ===========================================================================

function arr(...args) {
	return args;
}

// ===========================================================================

function int(val) {
	return Number(val);
}

// ===========================================================================

function float(val, fixed = 2) {
	return parseFloat((val).toFixed(fixed));
}

// ===========================================================================

function str(val) {
	return String(val);
}

// ===========================================================================

function v3(x, y, z) {
	return new Vec3(toFloat(x), toFloat(y), toFloat(z));
}

// ===========================================================================

function v2(x, y) {
	return new Vec2(x, y);
}

// ===========================================================================

function upper(val) {
	return String(val).toUpperCase();
}

// ===========================================================================

function lower(val) {
	return String(val).toLowerCase();
}

// ===========================================================================

function isNull(val) {
	if(val == null) {
		return true;
	}

	if(typeof val === "undefined") {
		return true;
	}

	return false;
}

// ===========================================================================

function distance(vec1, vec2) {
	if(isNull(vec1) || isNull(vec2)) {
		return false;
	}
    return vec1.distance(vec2);
}

// ===========================================================================

function o(params) {
	return getElementsByType(ELEMENT_OBJECT)[params];
}

// ===========================================================================

function pos(params) {
	return localPlayer.position;
}

// ===========================================================================

function posocb(params) {
	return `${localPlayer.position.x}, ${localPlayer.position.y}, ${localPlayer.position.z}`;
}

// ===========================================================================

function v(params) {
	return getElementsByType(ELEMENT_VEHICLE)[params];
}

// ===========================================================================

function closest(elementType, position = localPlayer.position) {
	return getElementsByType(elementType).reduce((i, j) => ((i.position.distance(position) <= j.position.distance(position)) ? i : j));
}

// ===========================================================================

function cv(elementType, position = localPlayer.position) {
	return closest(ELEMENT_VEHICLE, position);
}

// ===========================================================================

function co(elementType, position = localPlayer.position) {
	return closest(ELEMENT_OBJECT, position);
}

// ===========================================================================

function mv() {
	return localPlayer.vehicle;
}

// ===========================================================================

function right(pos, angle, distance) {
	let x = (pos.x+((Math.cos((-angle+1.57)+(Math.PI/2)))*distance));
	let y = (pos.y+((Math.sin((-angle+1.57)+(Math.PI/2)))*distance));

	let rightPos = new Vec3(x, y, pos.z);

	return rightPos;
}

// ===========================================================================

function left(pos, angle, distance) {
	let x = (pos.x+((Math.cos((angle+1.57)+(Math.PI/2)))*distance));
	let y = (pos.y+((Math.sin((angle+1.57)+(Math.PI/2)))*distance));

	let leftPos = new Vec3(x, y, pos.z);

	return leftPos;
}

// ===========================================================================

function front(pos, angle, distance) {
	let x = (pos.x+((Math.cos(angle+(Math.PI/2)))*distance));
	let y = (pos.y+((Math.sin(angle+(Math.PI/2)))*distance));
	let z = pos.z;

	return new Vec3(x, y, z);
}

// ===========================================================================

function behind(pos, angle, distance) {
	let x = (pos.x+((Math.cos(angle-(Math.PI/2)))*distance));
	let y = (pos.y+((Math.sin(angle-(Math.PI/2)))*distance));
	let z = pos.z;

	return new Vec3(x,y,z);
}

// ===========================================================================

function above(pos, distance) {
	return new Vec3(pos.x, pos.y, pos.z+distance);
}

// ===========================================================================

function below(pos, distance) {
	return new Vec3(pos.x, pos.y, pos.z-distance);
}

// ===========================================================================

function offset(position, position2) {
	return new Vec3(position.x+position2.x, position.y+position2.y, position.z+position2.z);
}

// ===========================================================================

function rand(min, max) {
	return Math.floor(Math.random() * (toInteger(max) - toInteger(min) + 1)) + toInteger(min)
}

// ===========================================================================

function ao(model, position) {
	let obj = gta.createObject(model, position);
	message(`Object added: ${obj.id}`);
	return obj;
}

// ===========================================================================