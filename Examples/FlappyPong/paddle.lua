
flapSpeed = 40.0
gravity = 50.0
isColliding = false
velocityY = 0.0

function OnEvent(component, name, args)
	if name == "Flap" then
		if velocityY < 0.0 then
			velocityY = 0.0
		end
		velocityY = velocityY + flapSpeed
		return
	end

	if name == "Tick" then
		local position = transform:GetPosition()
		local scale = transform:GetScale()
		if position.y <= -60.0 + scale.y and velocityY <= 0.0 then
			return
		end
		if position.y >= 60.0 - scale.y and velocityY >= 0.0 then
			velocityY = 0.0
		end
		if not isColliding then
			position.y = position.y + velocityY * args.deltaTime
			
			velocityY = velocityY - gravity * args.deltaTime
		end
		transform:SetPosition(position)
	end

	if name == "OnCollisionStart" then
		isColliding = true
	end

	if name == "OnCollisionEnd" then
		isColliding = false
	end
end
