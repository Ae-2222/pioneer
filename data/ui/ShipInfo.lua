local ui = Engine.ui

ui.templates.ShipInfo = function (args)
	local shipId = Game.player.shipType
	local shipType = ShipType.GetShipType(shipId)

	return ui:Background():SetInnerWidget(ui:Margin(30):SetInnerWidget(
		ui:Grid(2,1)
			:SetColumn(0, {
				ui:VBox(20):PackEnd({
					ui:Label("Ship information"):SetFontSize("LARGE"),
					ui:Grid(2,1)
						:SetColumn(0, {
							ui:VBox():PackEnd({
								ui:Label("Hyperdrive:"):SetFontSize("SMALL"),
								ui:Label("Hyperspace range:"):SetFontSize("SMALL"),
								ui:Margin(10),
								ui:Label("Capacity:"):SetFontSize("SMALL"),
								ui:Label("Free:"):SetFontSize("SMALL"),
								ui:Label("Used:"):SetFontSize("SMALL"),
								ui:Label("All-up weight:"):SetFontSize("SMALL"),
								ui:Margin(10),
								ui:Label("Front weapon:"):SetFontSize("SMALL"),
								ui:Label("Rear weapon:"):SetFontSize("SMALL")
							})
						})
				})
			})
			:SetColumn(1, {
				ui:VBox(10)
					:PackEnd(ui:Label(shipType.name))
					:PackEnd(UI.Game.ShipSpinner.New(ui, Game.player.shipType), { "EXPAND", "FILL" })
			})
	))
end
