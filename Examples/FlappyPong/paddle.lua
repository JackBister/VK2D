
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
		if transform.position.y <= -60.0 + transform.scale.y and velocityY <= 0.0 then
			return
		end
		if transform.position.y >= 60.0 - transform.scale.y and velocityY >= 0.0 then
			velocityY = 0.0
		end
		if not isColliding then
			transform.position.y = transform.position.y + velocityY * args.deltaTime
			transform.isToParentDirty = true
			transform.isToWorldDirty = true
			velocityY = velocityY - gravity * args.deltaTime
		end
	end

	if name == "OnCollisionStart" then
		isColliding = true
	end

	if name == "OnCollisionEnd" then
		isColliding = false
	end
end
