#include "sierrachart.h"
SCDLLName("MUSTR") 

SCSFExport scsf_TradingExampleWithAttachedOrdersUsingActualPrices(SCStudyInterfaceRef sc)
{
	// Define references to the Subgraphs and Inputs for easy reference
	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[0];
	SCSubgraphRef SellEntrySubgraph = sc.Subgraph[2];
	SCSubgraphRef SimpMovAvgSubgraph = sc.Subgraph[4];


	if (sc.SetDefaults)
	{
		// Set the study configuration and defaults.

		sc.GraphName = "Trading Example: With Attached Orders Using Actual Prices";

		BuyEntrySubgraph.Name = "Buy Entry";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
		BuyEntrySubgraph.DrawZeros = false;


		SellEntrySubgraph.Name = "Sell Entry";
		SellEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_DOWN;
		SellEntrySubgraph.PrimaryColor = RGB(255, 0, 0);
		SellEntrySubgraph.LineWidth = 2;
		SellEntrySubgraph.DrawZeros = false;


		SimpMovAvgSubgraph.Name = "Simple Moving Average";
		SimpMovAvgSubgraph.DrawStyle = DRAWSTYLE_LINE;
		SimpMovAvgSubgraph.DrawZeros = false;

		sc.StudyDescription = "This study function is an example of how to use the ACSIL Trading Functions.  This example uses Attached Orders defined directly within this function. This function will display a simple moving average and perform a Buy Entry when the Last price crosses the moving average from below and a Sell Entry when the Last price crosses the moving average from above. A new entry cannot occur until the Target or Stop has been filled. When an order is sent, a corresponding arrow will appear on the chart to show that an order was sent. This study will do nothing until the Enabled Input is set to Yes.";

		sc.AllowMultipleEntriesInSameDirection = false;

		//This must be equal to or greater than the order quantities you will be submitting orders for.
		sc.MaximumPositionAllowed = 5;

		sc.SupportReversals = false;

		// This is false by default. Orders will go to the simulation system always.
		sc.SendOrdersToTradeService = false;

		sc.AllowOppositeEntryWithOpposingPositionOrOrders = false;

		// This can be false in this function because we specify Attached Orders directly with the order which causes this to be considered true when submitting an order.
		sc.SupportAttachedOrdersForTrading = false;

		sc.CancelAllOrdersOnEntriesAndReversals = true;
		sc.AllowEntryWithWorkingOrders = false;
		sc.CancelAllWorkingOrdersOnExit = true;

		// Only 1 trade for each Order Action type is allowed per bar.
		sc.AllowOnlyOneTradePerBar = true;

		//This needs to be set to true when a trading study uses trading functions.
		sc.MaintainTradeStatisticsAndTradesData = true;

		sc.AutoLoop = 1;
		sc.GraphRegion = 0;

		// During development set this flag to 1, so the DLL can be modified. When development is completed, set it to 0 to improve performance.
		sc.FreeDLL = 1;

		return;
	}


	SCFloatArrayRef ClosePriceArray = sc.Close;

	// Use persistent variables to remember attached order IDs so they can be modified or canceled. 
	int& Target1OrderID = sc.GetPersistentInt(1);
	int& Stop1OrderID = sc.GetPersistentInt(2);

	// Calculate the moving average
	sc.SimpleMovAvg(ClosePriceArray, SimpMovAvgSubgraph, sc.Index, 10);


	// Create an s_SCNewOrder object. 
	s_SCNewOrder NewOrder;
	NewOrder.OrderQuantity = 1;
	NewOrder.OrderType = SCT_ORDERTYPE_MARKET;

	NewOrder.OCOGroup1Quantity = 1; // If this is left at the default of 0, then it will be automatically set.

									// Buy when the last price crosses the moving average from below.
	if (sc.CrossOver(ClosePriceArray, SimpMovAvgSubgraph) == CROSS_FROM_BOTTOM && sc.GetBarHasClosedStatus() == BHCS_BAR_HAS_CLOSED)
	{
		NewOrder.Target1Price = sc.Close[sc.Index] + 8 * sc.TickSize;
		NewOrder.Stop1Price = sc.Close[sc.Index] - 8 * sc.TickSize;

		int Result = sc.BuyEntry(NewOrder);
		if (Result > 0) //If there has been a successful order entry, then draw an arrow at the low of the bar.
		{
			BuyEntrySubgraph[sc.Index] = sc.Low[sc.Index];

			// Remember the order IDs for subsequent modification and cancellation
			Target1OrderID = NewOrder.Target1InternalOrderID;
			Stop1OrderID = NewOrder.Stop1InternalOrderID;
		}
	}


	// Sell when the last price crosses the moving average from above.
	else if (sc.CrossOver(ClosePriceArray, SimpMovAvgSubgraph) == CROSS_FROM_TOP && sc.GetBarHasClosedStatus() == BHCS_BAR_HAS_CLOSED)
	{

		NewOrder.Target1Price = sc.Close[sc.Index] - 8 * sc.TickSize;
		NewOrder.Stop1Price = sc.Close[sc.Index] + 8 * sc.TickSize;

		int Result = sc.SellEntry(NewOrder);
		if (Result > 0) //If there has been a successful order entry, then draw an arrow at the high of the bar.
		{
			SellEntrySubgraph[sc.Index] = sc.High[sc.Index];

			// Remember the order IDs for subsequent modification and cancellation
			Target1OrderID = NewOrder.Target1InternalOrderID;
			Stop1OrderID = NewOrder.Stop1InternalOrderID;
		}
	}


	// This code block serves as an example of how to modify an attached order.  In this example it is set to false and never runs.
	bool ExecuteModifyOrder = true;
	if (ExecuteModifyOrder && (sc.Index == sc.ArraySize - 1))//Only  do a modify on the most recent bar
	{
		double NewLimit = 0.0;

		// Get the existing target order
		s_SCTradeOrder ExistingOrder;
		if (sc.GetOrderByOrderID(Target1OrderID, ExistingOrder) != SCTRADING_ORDER_ERROR)
		{
			if (ExistingOrder.BuySell == BSE_BUY)
				NewLimit = sc.Close[sc.Index] - 5 * sc.TickSize;
			else if (ExistingOrder.BuySell == BSE_SELL)
				NewLimit = sc.Close[sc.Index] + 5 * sc.TickSize;

			// We can only modify price and/or quantity

			s_SCNewOrder ModifyOrder;
			ModifyOrder.InternalOrderID = Target1OrderID;
			ModifyOrder.Price1 = NewLimit;

			sc.ModifyOrder(ModifyOrder);
		}
	}
}