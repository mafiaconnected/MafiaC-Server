-- ----------------------------------------------------------------------------

addCommandHandler("lse", function(cmdName, params, client)
	if client.console == false then
		if client.administrator == false then
			return false
		end
	end

    if params == nil then
        messageClient("Syntax: /lse <code>", client, findResourceByName("v-utils").exports.getSyntaxMessageColour(gta.game))
        return false;
    end

	pcall(load(params))

	print("Lua server code executed: " .. params)

	messageClient("Lua server code executed: " .. params, client, COLOUR_AQUA)
end)

-- ----------------------------------------------------------------------------

addCommandHandler("lsr", function(cmdName, params, client)
	if client.console == false then
		if client.administrator == false then
			return false
		end
	end

    if params == nil then
        messageClient("Syntax: /lsr <code>", client, findResourceByName("v-utils").exports.getSyntaxMessageColour(gta.game))
        return false;
    end

	local output = pcall(load("return " .. params))

	print("Lua server code executed: " .. params)
	print("Returns: " .. output)

	messageClient("Lua server code executed: " .. output, COLOUR_AQUA)
	messageClient("Returns: " .. output, client, COLOUR_AQUA)


end)

-- ----------------------------------------------------------------------------