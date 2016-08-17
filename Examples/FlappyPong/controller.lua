
function OnEvent(component, name, args)
	if name == "BeginPlay" then
		leftPaddle = Scene:GetEntityByName("Left Paddle")
		rightPaddle = Scene:GetEntityByName("Right Paddle")
	end
	if name == "Tick" then
		if Input:GetButtonDown("Flap") then
			leftPaddle:FireEvent("Flap", {})
			rightPaddle:FireEvent("Flap", {})
		end
	end
end

