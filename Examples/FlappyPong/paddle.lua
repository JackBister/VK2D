
flapSpeed = 40.0
gravity = 50.0

velocityY = 0.0

function OnEvent(component, name, args)
	if name == "BeginPlay" then
		ball = Scene:GetEntityByName("Ball")
	end
	if name == "Flap" then
		if velocityY < 0.0 then
			velocityY = 0.0
		end
		velocityY = velocityY + flapSpeed
		return
	end

	if name == "Tick" then
		if CollisionCheck(ball.transform, transform) then
			ball:FireEvent("Bounce", {})
		end
		if transform.position.y <= -60.0 + transform.scale.y/2 and velocityY <= 0.0 then
			return
		end
		if transform.position.y >= 60.0 - transform.scale.y/2 and velocityY >= 0.0 then
			velocityY = 0.0
		end
		transform.position.y = transform.position.y + velocityY * args.deltaTime
		transform.isToParentDirty = true
		transform.isToWorldDirty = true
		velocityY = velocityY - gravity * args.deltaTime
	end
end

function CollisionCheck(t1, t2)
	if t1.position.x < t2.position.x and t1.position.x + t1.scale.x/2 >= t2.position.x - t2.scale.x/2 then
		return CheckY(t1, t2)	
	elseif t1.position.x > t2.position.x and t1.position.x - t1.scale.x/2 <= t2.position.x + t2.scale.x/2 then
		return CheckY(t1, t2)
	end
	return false
end

function CheckY(t1, t2)
	if t1.position.y < t2.position.y and t1.position.y + t1.scale.y/2 >= t2.position.y - t2.scale.y/2 then
		return true
	elseif t1.position.y > t2.position.y and t1.position.y - t1.scale.y/2 <= t2.position.y + t2.scale.y/2 then
		return true
	end
end

