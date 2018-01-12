
moveSpeed = 50.0

function Dot(a, b)
	return a.x * b.x + a.y * b.y
end

function Magnitude(v)
	return math.sqrt(Dot(v, v))
end

function Normalize(v)
	local mag = Magnitude(v)
	return {["x"] = v.x / mag, ["y"] = v.y / mag}
end

function VecMinus(a, b)
	return {["x"] = a.x - b.x, ["y"] = a.y - b.y}
end

function VecMultiply(a, b)
	return {["x"] = a * b.x, ["y"] = a * b.y}
end

function OnEvent(component, name, args)
	if name == "BeginPlay" then
		math.randomseed(os.time())
		velocityDir = { x = math.random()*2-1, y = math.random()*2-1 }
		if velocityDir.x < 0.0 then
			velocityDir.x = -1.0
		else
			velocityDir.x = 1.0
		end
	end

	if name == "Tick" then
		local position = transform:GetPosition()
		local scale = transform:GetScale()
		if position.y <= -60 + scale.y or position.y >= 60 - scale.y then
			velocityDir.y = -velocityDir.y
		end
		
		if position.x <= -80 - scale.x or position.x >= 80 + scale.x then
			position.x = 0
		end
		
		position.x = position.x + velocityDir.x * moveSpeed * args.deltaTime
		position.y = position.y + velocityDir.y * moveSpeed * args.deltaTime
		
		transform:SetPosition(position)
	end

	if name == "OnCollisionStart" then
		--This is a fallback for the situation where for some reason 0 normals exist for the collision.
		--I've only encountered this once in all my tests and could never reproduce it.
		if not args.info.normals then
			print("[WARNING] OnCollisionStart with no normal.")
			velocityDir.x = -velocityDir.x
		else
			local norm = Normalize(args.info.normals[1])
			velocityDir = VecMinus(velocityDir, VecMultiply(2 * Dot(velocityDir, norm), norm))
		end
	end
end

