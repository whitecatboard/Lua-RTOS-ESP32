-- the lua script must make sure to present the pin to the user
-- e.g. via attached display, etc.
net.wf.startwps(net.wf.wpstype.PIN, function (pin)
	print("Enter the PIN "..pin.." on your router.")
end)

