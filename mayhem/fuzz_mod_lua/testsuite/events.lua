local count = 0
Mod.On("FloorEnter", function(floor, n) Mod.Log("enter", floor, n, Mod.Floor(), Mod.Loop(), Mod.Step()) end)
Mod.On("Tick", function(dt) count = count + 1 end)
Mod.On("Message", function(name, sender) Mod.Log("message", name, sender, count, string.format("%d", 5)) end)
