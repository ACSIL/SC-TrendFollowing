#include "sierrachart.h"



//OBECNA FUNKCE PRO KORELACE
bool korelace(double H3, double H2, double H1, double L3, double L2, double L1)
{

	if ((H3 < H2) && (H2 < H1) && (L3 < L2) && (L2 < L1))

		return true;
}



SCDLLName("TREND FOLLOWING")
// (relativni meri odchylky range v %, absolutni je meri v ticich)
// rozvijim zatim jen tu absolutni verzi

//tuhle uy mam v 64 bit
SCSFExport scsf_TrendPattern_Relativni_00(SCStudyInterfaceRef sc)
{
	SCSubgraphRef Long = sc.Subgraph[2];
	SCSubgraphRef Short = sc.Subgraph[1];
	SCSubgraphRef EntryPointLong = sc.Subgraph[0];
	SCSubgraphRef ConsolidationBar = sc.Subgraph[3];

	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[10];
	SCSubgraphRef BuyExitSubgraph = sc.Subgraph[11];

	SCInputRef procenta = sc.Input[0];

	if (sc.SetDefaults)
	{
		sc.GraphName = "Trend pattern - Relativni (verze 1.0)";
		sc.StudyDescription = "vstupy do trendu na prekryvajici se barech (+ pak zapojim volume/deltu nebo market ordery)";
		sc.GraphRegion = 0;

		Long.Name = "Vstupni bar";
		Long.PrimaryColor = RGB(250, 250, 250);
		Long.DrawStyle = DRAWSTYLE_ARROWUP;
		Long.LineWidth = 5;

		EntryPointLong.Name = "Misto vstupu";
		EntryPointLong.PrimaryColor = RGB(250, 250, 250);
		EntryPointLong.DrawStyle = DRAWSTYLE_DASH;
		EntryPointLong.LineWidth = 5;

		BuyEntrySubgraph.Name = "Buy Entry";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
		BuyEntrySubgraph.DrawZeros = false;

		ConsolidationBar.Name = "Bar v konsolidaci";
		ConsolidationBar.PrimaryColor = RGB(128, 128, 255);
		ConsolidationBar.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBar.LineWidth = 2;

		sc.AutoLoop = 1;
		sc.FreeDLL = 1;

		sc.AllowMultipleEntriesInSameDirection = true;
		sc.MaximumPositionAllowed = 1000;
		sc.SupportReversals = false;
		sc.SendOrdersToTradeService = false;
		sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
		sc.SupportAttachedOrdersForTrading = true;
		sc.CancelAllOrdersOnEntriesAndReversals = false;
		sc.AllowEntryWithWorkingOrders = true;
		sc.CancelAllWorkingOrdersOnExit = false;
		sc.AllowOnlyOneTradePerBar = true;
		sc.MaintainTradeStatisticsAndTradesData = true;

		procenta.Name = "Odchylka v %";
		procenta.SetFloat(0.2f);
		procenta.SetFloatLimits(0.0f, 1.0f);

		return;
	}

	s_SCNewOrder Prikaz;
	Prikaz.OrderQuantity = 2;
	Prikaz.OrderType = SCT_ORDERTYPE_MARKET;

	// zakladam perzist vary pro uchovani ID pro pt a sl
	int& Target1OrderID = sc.GetPersistentInt(1);
	int& Stop1OrderID = sc.GetPersistentInt(2);

	//tohle nevim k cemu je.. (?)
	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);
	if (PositionData.PositionQuantity != 0)
		return;
	
	//specifikace oco pokynu
	Prikaz.Target1Offset = 10 * sc.TickSize;
	Prikaz.Target2Offset = 30 * sc.TickSize;
	Prikaz.StopAllPrice = sc.Low[sc.Index - 1] - 1 * sc.TickSize;

	Prikaz.OCOGroup1Quantity = 1; // tohle je co?? -- If this is left at the default of 0, then it will be automatically set.
								  
	//promenne pro range tech tri baru abych je mohl porovnat 
	float Range3 = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2 = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1 = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);
	
	//-----DEFINOVANI VSTUPNI PODMINKY-----//
	
		//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
	if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
		(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
		((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
		(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3]))) 
	{
		//pridavam podminku, že bary musi byt zhruba stejne dlouhé
		if ((Range2 < ((1 + procenta.GetFloat()) * Range3)) &&
			(Range2 > ((1 - procenta.GetFloat()) * Range3)) &&
			(Range1 < ((1 + procenta.GetFloat()) * Range3)) &&
			(Range1 > ((1 - procenta.GetFloat()) * Range3))) 
		{
			//a zaroven ten poslední musi uzavrit ve sve horni polovine
			if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
			{
				//vybarvi prislusne bary 
				ConsolidationBar[sc.Index - 1] = sc.Close[sc.Index - 1];
				ConsolidationBar[sc.Index - 2] = sc.Close[sc.Index - 2];
				ConsolidationBar[sc.Index - 3] = sc.Close[sc.Index - 3];
				//v polovine 3 z nich udelej carku
				EntryPointLong[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);
		
				//-----OTEVRENI POZICE-----//
				//kdyz se cena dotkne midu predchoziho baru
				if (sc.Close[sc.Index] < sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
				{	
					sc.BuyEntry(Prikaz);

				}
			}
		}
	}
}

//tuhle uy mam v 64 bit

SCSFExport scsf_TrendPattern_Absolutni_00(SCStudyInterfaceRef sc)
{
	SCSubgraphRef Long = sc.Subgraph[2];
	SCSubgraphRef Short = sc.Subgraph[1];
	SCSubgraphRef EntryPointLong = sc.Subgraph[0];
	SCSubgraphRef ConsolidationBar = sc.Subgraph[3];

	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[10];
	SCSubgraphRef BuyExitSubgraph = sc.Subgraph[11];

	SCInputRef odchylka = sc.Input[0];
	SCInputRef zacatek_rth = sc.Input[1];
	SCInputRef konec_rth = sc.Input[2];


	if (sc.SetDefaults)
	{
		sc.GraphName = "Trend pattern - Absolutni (verze 1.0)";
		sc.StudyDescription = "vstupy do trendu na prekryvajici se barech (+ pak zapojim volume/deltu nebo market ordery)";
		sc.GraphRegion = 0;

		Long.Name = "Vstupni bar";
		Long.PrimaryColor = RGB(250, 250, 250);
		Long.DrawStyle = DRAWSTYLE_ARROWUP;
		Long.LineWidth = 5;

		EntryPointLong.Name = "Misto vstupu";
		EntryPointLong.PrimaryColor = RGB(250, 250, 250);
		EntryPointLong.DrawStyle = DRAWSTYLE_DASH;
		EntryPointLong.LineWidth = 5;

		BuyEntrySubgraph.Name = "Buy Entry";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
		BuyEntrySubgraph.DrawZeros = false;

		ConsolidationBar.Name = "Bar v konsolidaci";
		ConsolidationBar.PrimaryColor = RGB(128, 128, 255);
		ConsolidationBar.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBar.LineWidth = 2;

		//nastaveni casu
		zacatek_rth.Name = "Trade From:";
		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));

		konec_rth.Name = "Trade Till:";
		konec_rth.SetTime(HMS_TIME(15, 10, 00));
	
		sc.AutoLoop = 1;
		sc.FreeDLL = 1;

		sc.AllowMultipleEntriesInSameDirection = true;
		sc.MaximumPositionAllowed = 1000;
		sc.SupportReversals = false;
		sc.SendOrdersToTradeService = false;
		sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
		sc.SupportAttachedOrdersForTrading = true;
		sc.CancelAllOrdersOnEntriesAndReversals = false;
		sc.AllowEntryWithWorkingOrders = true;
		sc.CancelAllWorkingOrdersOnExit = false;
		sc.AllowOnlyOneTradePerBar = true;
		sc.MaintainTradeStatisticsAndTradesData = true;

		odchylka.Name = "Odchylka v ticich";
		odchylka.SetInt(3);
		
		return;
	}

	s_SCNewOrder Prikaz;
	Prikaz.OrderQuantity = 2;
	Prikaz.OrderType = SCT_ORDERTYPE_MARKET;

	// zakladam perzist vary pro uchovani ID pro pt a sl
	int& Target1OrderID = sc.GetPersistentInt(1);
	int& Stop1OrderID = sc.GetPersistentInt(2);
		
	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);

	//jsem li v pozici, dalsi neotevirej
	if (PositionData.PositionQuantity != 0) return;	
	
	//specifikace oco pokynu
	Prikaz.Target1Offset = 100 * sc.TickSize;
	Prikaz.Target2Offset = 300 * sc.TickSize;
	Prikaz.StopAllPrice = sc.Low[sc.Index - 1] - 100 * sc.TickSize;
	Prikaz.OCOGroup1Quantity = 1; // tohle je co?? -- If this is left at the default of 0, then it will be automatically set.

	 //promenne pro range tech tri baru abych je mohl porovnat 
	float Range3 = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2 = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1 = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

								  //-----DEFINOVANI VSTUPNI PODMINKY-----//

	//jsme li v obchodnich hodinach
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime()) 
	{
		//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
		if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
		(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
		((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
		(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3])))
			{
			//pridavam podminku, že bary musi byt zhruba stejne dlouhé 
			if ((Range2 < (Range3 + odchylka.GetInt()*sc.TickSize)) &&
			(Range2 > (Range3 - odchylka.GetInt()*sc.TickSize)) &&
			(Range1 < (Range3 + odchylka.GetInt()*sc.TickSize)) &&
			(Range1 > (Range3 - odchylka.GetInt()*sc.TickSize)))
				{
				//a zaroven ten poslední musi uzavrit ve sve horni polovine
				if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
				{
				//vybarvi prislusne bary 
				ConsolidationBar[sc.Index - 1] = sc.Close[sc.Index - 1];
				ConsolidationBar[sc.Index - 2] = sc.Close[sc.Index - 2];
				ConsolidationBar[sc.Index - 3] = sc.Close[sc.Index - 3];
				//v polovine 3 z nich udelej carku
				EntryPointLong[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);
				
					//-----OTEVRENI POZICE-----//
					//kdyz se cena dotkne midu predchoziho baru
					if (sc.Close[sc.Index] < sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
					sc.BuyEntry(Prikaz);

					}
				}
			}
		}

	}

	//---------RESENI VYSTUPU NA KONCI DNE----------

	int velikost_pozice = PositionData.PositionQuantity;

	if (PositionData.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= konec_rth.GetTime())
	{
		sc.FlattenAndCancelAllOrders();
	}
}

SCSFExport scsf_TrendPattern_Absolutni_01(SCStudyInterfaceRef sc)
{
	SCSubgraphRef Long = sc.Subgraph[2];
	SCSubgraphRef Short = sc.Subgraph[1];
	SCSubgraphRef EntryPointLong = sc.Subgraph[0];
	SCSubgraphRef ConsolidationBar = sc.Subgraph[3];

	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[10];
	SCSubgraphRef BuyExitSubgraph = sc.Subgraph[11];

	SCInputRef odchylka = sc.Input[0];
	SCInputRef zacatek_rth = sc.Input[1];
	SCInputRef konec_rth = sc.Input[2];


	if (sc.SetDefaults)
	{
		sc.GraphName = "Trend pattern - Absolutni (verze 2.0)";
		sc.StudyDescription = "vstupy do trendu na prekryvajici se barech (+ pak zapojim volume/deltu nebo market ordery)";
		sc.GraphRegion = 0;

		Long.Name = "Vstupni bar";
		Long.PrimaryColor = RGB(250, 250, 250);
		Long.DrawStyle = DRAWSTYLE_ARROWUP;
		Long.LineWidth = 5;

		EntryPointLong.Name = "Misto vstupu";
		EntryPointLong.PrimaryColor = RGB(250, 250, 250);
		EntryPointLong.DrawStyle = DRAWSTYLE_DASH;
		EntryPointLong.LineWidth = 5;

		BuyEntrySubgraph.Name = "Buy Entry";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
		BuyEntrySubgraph.DrawZeros = false;

		ConsolidationBar.Name = "Bar v konsolidaci";
		ConsolidationBar.PrimaryColor = RGB(128, 128, 255);
		ConsolidationBar.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBar.LineWidth = 2;

		//nastaveni casu
		zacatek_rth.Name = "Trade From:";
		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));

		konec_rth.Name = "Trade Till:";
		konec_rth.SetTime(HMS_TIME(15, 10, 00));

		sc.AutoLoop = 1;
		sc.FreeDLL = 1;

		sc.AllowMultipleEntriesInSameDirection = true;
		sc.MaximumPositionAllowed = 1000;
		sc.SupportReversals = false;
		sc.SendOrdersToTradeService = false;
		sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
		sc.SupportAttachedOrdersForTrading = true;
		sc.CancelAllOrdersOnEntriesAndReversals = false;
		sc.AllowEntryWithWorkingOrders = true;
		sc.CancelAllWorkingOrdersOnExit = false;
		sc.AllowOnlyOneTradePerBar = true;
		sc.MaintainTradeStatisticsAndTradesData = true;

		odchylka.Name = "Odchylka v ticich";
		odchylka.SetInt(3);

		return;
	}

	s_SCNewOrder Prikaz;
	Prikaz.OrderQuantity = 2;
	Prikaz.OrderType = SCT_ORDERTYPE_MARKET;

	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);

	//jsem li v pozici, dalsi neotevirej
	if (PositionData.PositionQuantity != 0) return;

	//specifikace oco pokynu
	int SLVTicich = (sc.High[sc.Index - 1] - sc.Low[sc.Index - 3]) / 2 / sc.TickSize;
	Prikaz.Target1Offset = SLVTicich * sc.TickSize;
	Prikaz.Target2Offset = (SLVTicich * sc.TickSize)*3;
	Prikaz.StopAllOffset = SLVTicich * sc.TickSize;

	Prikaz.OCOGroup1Quantity = 1; // tohle je co?? -- If this is left at the default of 0, then it will be automatically set.

								  //promenne pro range tech tri baru abych je mohl porovnat 
	float Range3 = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2 = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1 = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

	//-----DEFINOVANI VSTUPNI PODMINKY-----//

	//jsme li v obchodnich hodinach
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime())
	{
		//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
		if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že bary musi byt zhruba stejne dlouhé 
			if ((Range2 < (Range3 + odchylka.GetInt()*sc.TickSize)) &&
				(Range2 >(Range3 - odchylka.GetInt()*sc.TickSize)) &&
				(Range1 < (Range3 + odchylka.GetInt()*sc.TickSize)) &&
				(Range1 >(Range3 - odchylka.GetInt()*sc.TickSize)))
			{
				//a zaroven ten poslední musi uzavrit ve sve horni polovine
				if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
				{
					//vybarvi prislusne bary 
					ConsolidationBar[sc.Index - 1] = sc.Close[sc.Index - 1];
					ConsolidationBar[sc.Index - 2] = sc.Close[sc.Index - 2];
					ConsolidationBar[sc.Index - 3] = sc.Close[sc.Index - 3];
					//v polovine 3 z nich udelej carku
					EntryPointLong[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

					//-----OTEVRENI POZICE-----//
					//kdyz se cena dotkne midu predchoziho baru
					if (sc.Close[sc.Index] < sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						sc.BuyEntry(Prikaz);

					}
				}
			}
		}

	}

	//---------RESENI VYSTUPU NA KONCI DNE----------

	//int velikost_pozice = PositionData.PositionQuantity;

	if (PositionData.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= konec_rth.GetTime())
	{
		sc.FlattenAndCancelAllOrders();
	}
}

SCSFExport scsf_TrendPattern_Absolutni_02(SCStudyInterfaceRef sc)
{
	SCSubgraphRef Long = sc.Subgraph[2];
	SCSubgraphRef Short = sc.Subgraph[1];
	SCSubgraphRef EntryPointLong = sc.Subgraph[0];
	SCSubgraphRef ConsolidationBar = sc.Subgraph[3];

	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[10];
	SCSubgraphRef BuyExitSubgraph = sc.Subgraph[11];

	SCInputRef odchylka = sc.Input[0];
	SCInputRef zacatek_rth = sc.Input[1];
	SCInputRef konec_rth = sc.Input[2];


	if (sc.SetDefaults)
	{
		sc.GraphName = "Trend pattern - Absolutni (Verze 3.0)";
		sc.StudyDescription = "vstupy do trendu na prekryvajici se barech (+ pak zapojim volume/deltu nebo market ordery)";
		sc.GraphRegion = 0;

		Long.Name = "Vstupni bar";
		Long.PrimaryColor = RGB(250, 250, 250);
		Long.DrawStyle = DRAWSTYLE_ARROWUP;
		Long.LineWidth = 5;

		EntryPointLong.Name = "Misto vstupu";
		EntryPointLong.PrimaryColor = RGB(250, 250, 250);
		EntryPointLong.DrawStyle = DRAWSTYLE_DASH;
		EntryPointLong.LineWidth = 5;

		BuyEntrySubgraph.Name = "Buy Entry";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
		BuyEntrySubgraph.DrawZeros = false;

		ConsolidationBar.Name = "Bar v konsolidaci";
		ConsolidationBar.PrimaryColor = RGB(128, 128, 255);
		ConsolidationBar.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBar.LineWidth = 2;

		//nastaveni casu
		zacatek_rth.Name = "Trade From:";
		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));

		konec_rth.Name = "Trade Till:";
		konec_rth.SetTime(HMS_TIME(15, 10, 00));

		sc.AutoLoop = 1;
		sc.FreeDLL = 1;

		sc.AllowMultipleEntriesInSameDirection = true;
		sc.MaximumPositionAllowed = 1000;
		sc.SupportReversals = false;
		sc.SendOrdersToTradeService = false;
		sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
		sc.SupportAttachedOrdersForTrading = true;
		sc.CancelAllOrdersOnEntriesAndReversals = false;
		sc.AllowEntryWithWorkingOrders = true;
		sc.CancelAllWorkingOrdersOnExit = false;
		sc.AllowOnlyOneTradePerBar = true;
		sc.MaintainTradeStatisticsAndTradesData = true;

		odchylka.Name = "Odchylka v ticich";
		odchylka.SetInt(3);

		return;
	}

	s_SCNewOrder Prikaz;
	Prikaz.OrderQuantity = 2;
	Prikaz.OrderType = SCT_ORDERTYPE_MARKET;

	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);

	//jsem li v pozici, dalsi neotevirej
	if (PositionData.PositionQuantity != 0) return;

	//specifikace oco pokynu
	int SLVTicich = (sc.High[sc.Index - 1] - sc.Low[sc.Index - 3]) / 2 / sc.TickSize;
	Prikaz.Target1Offset = SLVTicich * sc.TickSize;
	Prikaz.Target2Offset = (SLVTicich * sc.TickSize) * 3;
	Prikaz.StopAllOffset = SLVTicich * sc.TickSize;

	Prikaz.OCOGroup1Quantity = 1; // tohle je co?? -- If this is left at the default of 0, then it will be automatically set.

								  //promenne pro range tech tri baru abych je mohl porovnat 
	float Range3 = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2 = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1 = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

	//-----DEFINOVANI VSTUPNI PODMINKY-----//

	//jsme li v obchodnich hodinach
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime())
	{
		//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
		if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že bary musi byt zhruba stejne dlouhé 
			if ((Range2 < (Range3 + odchylka.GetInt()*sc.TickSize)) &&
				(Range2 > (Range3 - odchylka.GetInt()*sc.TickSize)) &&
				(Range1 < (Range3 + odchylka.GetInt()*sc.TickSize)) &&
				(Range1 > (Range3 - odchylka.GetInt()*sc.TickSize)))
			{	//kazdý low nasledujiciho bar se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
				if (sc.Low[sc.Index - 1] <= sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]) &&
					sc.Low[sc.Index - 2] <= sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]))
				{ 			
					//a zaroven ten poslední musi uzavrit ve sve horni polovine
					if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						//vybarvi prislusne bary 
						ConsolidationBar[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBar[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBar[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3 z nich udelej carku
						EntryPointLong[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

						//-----OTEVRENI POZICE-----//
						//kdyz se cena dotkne midu predchoziho baru
						if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
						{
							sc.BuyEntry(Prikaz);
							
						}
					}
				}
			}
		}

	}

	//---------RESENI VYSTUPU NA KONCI DNE----------

	//int velikost_pozice = PositionData.PositionQuantity;

	if (PositionData.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= konec_rth.GetTime())
	{
		sc.FlattenAndCancelAllOrders();
	}
}

SCSFExport scsf_TrendPattern_Absolutni_03(SCStudyInterfaceRef sc)
{

	SCSubgraphRef EntryPoint = sc.Subgraph[0];
	SCSubgraphRef ConsolidationBarLong = sc.Subgraph[3];
	SCSubgraphRef ConsolidationBarShort = sc.Subgraph[4];

	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[10];
	SCSubgraphRef BuyExitSubgraph = sc.Subgraph[11];

	SCInputRef zacatek_rth = sc.Input[1];
	SCInputRef konec_rth = sc.Input[2];
	SCInputRef tolerance = sc.Input[3];
	SCInputRef odchylka = sc.Input[4];

	SCInputRef TypVystupuOCOVar = sc.Input[10];
	SCInputRef TypVystupuOCOFix = sc.Input[11];

	SCInputRef MaximumPositionAllowed = sc.Input[30];
	SCInputRef PositionQuantity = sc.Input[31];
	SCInputRef AllowEntryWithWorkingOrders = sc.Input[32];
	SCInputRef SupportReversals = sc.Input[33];
	SCInputRef SendOrdersToTradeService = sc.Input[34];
	SCInputRef AllowMultipleEntriesInSameDirection = sc.Input[35];
	SCInputRef AllowOppositeEntryWithOpposingPositionOrOrders = sc.Input[36];
	SCInputRef CancelAllOrdersOnEntriesAndReversals = sc.Input[38];
	SCInputRef CancelAllWorkingOrdersOnExit = sc.Input[39];
	SCInputRef AllowOnlyOneTradePerBar = sc.Input[40];
	SCInputRef SupportAttachedOrdersForTrading = sc.Input[41];

	if (sc.SetDefaults)
	{
		sc.GraphName = "Trend pattern - Absolutni (verze 4.2 - pridany inputy pro nastaveni variance a tolerance, odladeny LOG )";
		sc.StudyDescription = "vstupy do trendu na prekryvajici se barech (+ pak zapojim volume/deltu nebo market ordery)";
		sc.GraphRegion = 0;

		EntryPoint.Name = "Misto vstupu";
		EntryPoint.PrimaryColor = RGB(250, 250, 250);
		EntryPoint.DrawStyle = DRAWSTYLE_DASH;
		EntryPoint.LineWidth = 5;

		BuyEntrySubgraph.Name = "Buy Entry";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
		BuyEntrySubgraph.DrawZeros = false;

		ConsolidationBarLong.Name = "Bar v konsolidaci";
		ConsolidationBarLong.PrimaryColor = RGB(128, 128, 255);
		ConsolidationBarLong.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBarLong.LineWidth = 2;

		ConsolidationBarShort.Name = "Bar v konsolidaci";
		ConsolidationBarShort.PrimaryColor = RGB(255, 255, 0);
		ConsolidationBarShort.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBarShort.LineWidth = 2;

		//nastaveni casu
		zacatek_rth.Name = "Trade From:";
		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));

		konec_rth.Name = "Trade Till:";
		konec_rth.SetTime(HMS_TIME(15, 11, 00));

		//nastaveni objemu 
		PositionQuantity.Name = "Position Quantity";
		PositionQuantity.SetInt(2);

		MaximumPositionAllowed.Name = "Maximum Position Allowed";
		MaximumPositionAllowed.SetInt(6);

		//nastaveni boolu pro trading
		AllowEntryWithWorkingOrders.Name = "Allow Entry With Working Orders (Yes/No)?";
		AllowEntryWithWorkingOrders.SetYesNo(false);

		SupportReversals.Name = "Support Reversals?";
		SupportReversals.SetYesNo(false);

		SendOrdersToTradeService.Name = "Send Orders To Trade Service?";
		SendOrdersToTradeService.SetYesNo(false);

		AllowMultipleEntriesInSameDirection.Name = "Allow Multiple Entries In Same Direction";
		AllowMultipleEntriesInSameDirection.SetYesNo(false);

		AllowOppositeEntryWithOpposingPositionOrOrders.Name = "Allow Opposite Entry With Opposing Position Or Orders";
		AllowOppositeEntryWithOpposingPositionOrOrders.SetYesNo(false);

		CancelAllOrdersOnEntriesAndReversals.Name = "Cancel All Orders On Entries And Reversals";
		CancelAllOrdersOnEntriesAndReversals.SetYesNo(false);

		CancelAllWorkingOrdersOnExit.Name = "Cancel All Working Orders On Exit";
		CancelAllWorkingOrdersOnExit.SetYesNo(true);

		AllowOnlyOneTradePerBar.Name = "Allow Only One Trade Per Bar";
		AllowOnlyOneTradePerBar.SetYesNo(true);

		SupportAttachedOrdersForTrading.Name = "Support Attached Orders For Trading";
		SupportAttachedOrdersForTrading.SetYesNo(false);

		odchylka.Name = "Variance Of The Bars´ Range (in ticks)";
		odchylka.SetInt(5);

		tolerance.Name = "Tolerance For The Previous Bars´ Mid Touch (in ticks)";
		tolerance.SetInt(0);

		TypVystupuOCOVar.Name = "Typ vystupu: OCO variabilni dle range";
		TypVystupuOCOVar.SetYesNo(true);

		TypVystupuOCOFix.Name = "Typ vystupu: OCO fixne";
		TypVystupuOCOFix.SetYesNo(false);

		sc.AutoLoop = 1;
		sc.FreeDLL = 1;

		return;
	}

	sc.MaximumPositionAllowed = MaximumPositionAllowed.GetInt(); //ok
	sc.AllowEntryWithWorkingOrders = AllowEntryWithWorkingOrders.GetInt(); //ok
	sc.SupportReversals = SupportReversals.GetYesNo(); //ok
	sc.SendOrdersToTradeService = SendOrdersToTradeService.GetYesNo(); //ok
	sc.AllowMultipleEntriesInSameDirection = AllowMultipleEntriesInSameDirection.GetYesNo(); //ok
	sc.AllowOppositeEntryWithOpposingPositionOrOrders = AllowOppositeEntryWithOpposingPositionOrOrders.GetYesNo(); //ok
	sc.SupportAttachedOrdersForTrading = SupportAttachedOrdersForTrading.GetYesNo(); //ok
	sc.CancelAllOrdersOnEntriesAndReversals = CancelAllOrdersOnEntriesAndReversals.GetYesNo(); //ok;
	sc.CancelAllWorkingOrdersOnExit = CancelAllWorkingOrdersOnExit.GetYesNo(); //ok
	sc.AllowOnlyOneTradePerBar = AllowOnlyOneTradePerBar.GetYesNo(); //ok
	sc.SupportAttachedOrdersForTrading = SupportAttachedOrdersForTrading.GetYesNo();//ok
	sc.MaintainTradeStatisticsAndTradesData = true;

	//-------------DEFINOVANI POKYNU---------//
		
	//specifikace variabilnich oco vazeb pro long
	s_SCNewOrder PrikazLongOCOVar;
	PrikazLongOCOVar.OrderQuantity = 2;
	PrikazLongOCOVar.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_long = (sc.High[sc.Index - 1] - sc.Low[sc.Index - 3]) / 2 / sc.TickSize; //dynamicky pocitany sl a pt pro long
	PrikazLongOCOVar.Target1Offset = sl_pt_long * sc.TickSize;
	PrikazLongOCOVar.Target2Offset = (sl_pt_long * sc.TickSize) * 3;
	PrikazLongOCOVar.StopAllOffset = sl_pt_long * sc.TickSize;

	//specifikace fixnich oco vazeb pro long
	s_SCNewOrder PrikazLongOCOFix;
	PrikazLongOCOFix.OrderQuantity = 2;
	PrikazLongOCOFix.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazLongOCOFix.Target1Offset = 20 * sc.TickSize;
	PrikazLongOCOFix.Target2Offset = 40 * sc.TickSize;
	PrikazLongOCOFix.StopAllOffset = 20 * sc.TickSize;

	//specifikace variabilnich oco vazeb pro short
	s_SCNewOrder PrikazShortOCOVar;
	PrikazShortOCOVar.OrderQuantity = 2;
	PrikazShortOCOVar.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_short = abs(sc.Low[sc.Index - 1] - sc.High[sc.Index - 3]) / 2 / sc.TickSize; //dynamicky pocitany sl a pt pro short
	PrikazShortOCOVar.Target1Offset = sl_pt_short * sc.TickSize;
	PrikazShortOCOVar.Target2Offset = (sl_pt_short * sc.TickSize) * 3;
	PrikazShortOCOVar.StopAllOffset = sl_pt_short * sc.TickSize;
	
	//specifikace fixnich oco vazeb pro short
	s_SCNewOrder PrikazShortOCOFix;
	PrikazShortOCOFix.OrderQuantity = 2;
	PrikazShortOCOFix.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazShortOCOFix.Target1Offset = 20 * sc.TickSize;
	PrikazShortOCOFix.Target2Offset = 40 * sc.TickSize;
	PrikazShortOCOFix.StopAllOffset = 20 * sc.TickSize;
	

	//promenne pro range tri baru RUSTOVYCH abych je mohl porovnat 
	float Range3Long = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2Long = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1Long = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);
	//promenne pro range tri baru KLESAJICICH abych je mohl porovnat 
	float Range3Short = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2Short = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1Short = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);
						
	//-----DEFINOVANI VSTUPNI PODMINKY-----//

	//jsme li v obchodnich hodinach
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime())
	{
		//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
		if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že bary musi byt zhruba stejne dlouhé 
			if ((Range2Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
				(Range2Long >(Range3Long - odchylka.GetInt()*sc.TickSize)) &&
				(Range1Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
				(Range1Long > (Range3Long - odchylka.GetInt()*sc.TickSize)))
			{	
				//kazdý low nasledujiciho bar se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
				if (sc.Low[sc.Index - 1] <= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2])) + tolerance.GetInt()*sc.TickSize  &&
					sc.Low[sc.Index - 2] <= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) + tolerance.GetInt()*sc.TickSize)
				{
					//a zaroven ten poslední musi uzavrit ve sve horni polovine
					if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						//vybarvi prislusne bary modre
						ConsolidationBarLong[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBarLong[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBarLong[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3. z nich udelej carku
						EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

						//-----OTEVRENI POZICE LONG-----//
						//kdyz se cena dotkne midu predchoziho baru
						if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOVar);
						}
						else if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOFix.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOFix);
						}
					}
				}
			}
		}

		//vytvarim sekvenci 3 baru ktery klesaji, kazdy ma nizsi highlow nez predchazejici 
		else if (((sc.High[sc.Index - 1] < sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] < sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] < sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] < sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že ty klesajici bary musi byt zhruba stejne dlouhé 
			if ((Range2Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
				(Range2Short >(Range3Short - odchylka.GetInt()*sc.TickSize)) &&
				(Range1Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
				(Range1Short >(Range3Short - odchylka.GetInt()*sc.TickSize)))
			{
				//dalsi pominka je, ze kazdy high nasledujiciho baru se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
				if (sc.High[sc.Index - 1] >= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]) - tolerance.GetInt()*sc.TickSize) &&
					sc.High[sc.Index - 2] >= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) - tolerance.GetInt()*sc.TickSize)
				{
					//a zaroven ten poslední musi uzavrit ve sve spodni polovine
					if (sc.Close[sc.Index - 1] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						//vybarvi prislusne bary zlute
						ConsolidationBarShort[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBarShort[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBarShort[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3. z nich udelej carku
						EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

						//-----OTEVRENI POZICE SHORT-----//
						//kdyz se cena dotkne midu predchoziho baru

						if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOVar);
						}
						else if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOFix.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOFix);
						}
					} 
				}
			}
		}
	}						

		
	//---------RESENI VYSTUPU NA KONCI DNE----------

	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);
	int velikost_pozice = PositionData.PositionQuantity;

	if (velikost_pozice != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= konec_rth.GetTime())
	{
		sc.FlattenAndCancelAllOrders();
	}
}

SCSFExport scsf_TrendPattern_Absolutni_04(SCStudyInterfaceRef sc)
{

	SCSubgraphRef EntryPoint = sc.Subgraph[0];
	SCSubgraphRef ConsolidationBarLong = sc.Subgraph[3];
	SCSubgraphRef ConsolidationBarShort = sc.Subgraph[4];

	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[10];
	SCSubgraphRef BuyExitSubgraph = sc.Subgraph[11];

	SCInputRef zacatek_rth = sc.Input[1];
	SCInputRef konec_rth = sc.Input[2];
	SCInputRef tolerance = sc.Input[3];
	SCInputRef odchylka = sc.Input[4];

	SCInputRef TypVystupuOCOVar = sc.Input[10];
	SCInputRef TypVystupuOCOFix = sc.Input[11];
	
	SCInputRef OCOFixSL = sc.Input[20];
	SCInputRef OCOFixPT1 = sc.Input[21];
	SCInputRef OCOFixPT2 = sc.Input[22];

	SCInputRef MaximumPositionAllowed = sc.Input[30];
	SCInputRef PositionQuantity = sc.Input[31];
	SCInputRef AllowEntryWithWorkingOrders = sc.Input[32];
	SCInputRef SupportReversals = sc.Input[33];
	SCInputRef SendOrdersToTradeService = sc.Input[34];
	SCInputRef AllowMultipleEntriesInSameDirection = sc.Input[35];
	SCInputRef AllowOppositeEntryWithOpposingPositionOrOrders = sc.Input[36];
	SCInputRef CancelAllOrdersOnEntriesAndReversals = sc.Input[38];
	SCInputRef CancelAllWorkingOrdersOnExit = sc.Input[39];
	SCInputRef AllowOnlyOneTradePerBar = sc.Input[40];
	SCInputRef SupportAttachedOrdersForTrading = sc.Input[41];

	SCInputRef EvaluateOnBarCloseOnly = sc.Input[50];

	if (sc.SetDefaults)
	{
		sc.GraphName = "Trend pattern - Absolutni (verze 4.3 - pridan vstup na close baru)";
		sc.StudyDescription = "vstupy do trendu na prekryvajici se barech (+ pak zapojim volume/deltu nebo market ordery)";
		sc.GraphRegion = 0;

		EntryPoint.Name = "Misto vstupu";
		EntryPoint.PrimaryColor = RGB(250, 250, 250);
		EntryPoint.DrawStyle = DRAWSTYLE_DASH;
		EntryPoint.LineWidth = 5;

		BuyEntrySubgraph.Name = "Buy Entry";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
		BuyEntrySubgraph.DrawZeros = false;

		ConsolidationBarLong.Name = "Bar v konsolidaci";
		ConsolidationBarLong.PrimaryColor = RGB(128, 128, 255);
		ConsolidationBarLong.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBarLong.LineWidth = 2;

		ConsolidationBarShort.Name = "Bar v konsolidaci";
		ConsolidationBarShort.PrimaryColor = RGB(255, 255, 0);
		ConsolidationBarShort.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBarShort.LineWidth = 2;

		//nastaveni casu
		zacatek_rth.Name = "Trade From:";
		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));

		konec_rth.Name = "Trade Till:";
		konec_rth.SetTime(HMS_TIME(15, 10, 00));

		//nastaveni objemu 
		PositionQuantity.Name = "Position Quantity";
		PositionQuantity.SetInt(2);

		MaximumPositionAllowed.Name = "Maximum Position Allowed";
		MaximumPositionAllowed.SetInt(6);

		//nastaveni boolu pro trading
		AllowEntryWithWorkingOrders.Name = "Allow Entry With Working Orders (Yes/No)?";
		AllowEntryWithWorkingOrders.SetYesNo(false);

		SupportReversals.Name = "Support Reversals?";
		SupportReversals.SetYesNo(false);

		SendOrdersToTradeService.Name = "Send Orders To Trade Service?";
		SendOrdersToTradeService.SetYesNo(false);

		AllowMultipleEntriesInSameDirection.Name = "Allow Multiple Entries In Same Direction";
		AllowMultipleEntriesInSameDirection.SetYesNo(false);

		AllowOppositeEntryWithOpposingPositionOrOrders.Name = "Allow Opposite Entry With Opposing Position Or Orders";
		AllowOppositeEntryWithOpposingPositionOrOrders.SetYesNo(false);

		CancelAllOrdersOnEntriesAndReversals.Name = "Cancel All Orders On Entries And Reversals";
		CancelAllOrdersOnEntriesAndReversals.SetYesNo(false);

		CancelAllWorkingOrdersOnExit.Name = "Cancel All Working Orders On Exit";
		CancelAllWorkingOrdersOnExit.SetYesNo(true);

		AllowOnlyOneTradePerBar.Name = "Allow Only One Trade Per Bar";
		AllowOnlyOneTradePerBar.SetYesNo(false);

		SupportAttachedOrdersForTrading.Name = "Support Attached Orders For Trading";
		SupportAttachedOrdersForTrading.SetYesNo(true);

		//nastavitelnme tolerance
		odchylka.Name = "Variance Of The Bars´ Range (in ticks)";
		odchylka.SetInt(5);

		tolerance.Name = "Tolerance For The Previous Bars´ Mid Touch (in ticks)";
		tolerance.SetInt(0);

		//nastavitelne vstupy
		EvaluateOnBarCloseOnly.Name = "Evaluate on Bar Close Only";
		EvaluateOnBarCloseOnly.SetYesNo(false);

		//nastavitelne vystupy
		TypVystupuOCOVar.Name = "Exit type: OCO Range Based Variable";
		TypVystupuOCOVar.SetYesNo(true);

		TypVystupuOCOFix.Name = "Exit type: OCO Fixed";
		TypVystupuOCOFix.SetYesNo(false);

		OCOFixSL.Name = "Set SL for OCO Fixed (in ticks)";
		OCOFixSL.SetInt(15);

		OCOFixPT1.Name = "Set 1. PT for OCO Fixed (in ticks)";
		OCOFixPT1.SetInt(15);

		OCOFixPT2.Name = "Set 2. PT for OCO Fixed (in ticks)";
		OCOFixPT2.SetInt(40);

		sc.AutoLoop = 1;
		sc.FreeDLL = 1;

		return;
	}

	sc.MaximumPositionAllowed = MaximumPositionAllowed.GetInt(); //ok
	sc.AllowEntryWithWorkingOrders = AllowEntryWithWorkingOrders.GetInt(); //ok
	sc.SupportReversals = SupportReversals.GetYesNo(); //ok
	sc.SendOrdersToTradeService = SendOrdersToTradeService.GetYesNo(); //ok
	sc.AllowMultipleEntriesInSameDirection = AllowMultipleEntriesInSameDirection.GetYesNo(); //ok
	sc.AllowOppositeEntryWithOpposingPositionOrOrders = AllowOppositeEntryWithOpposingPositionOrOrders.GetYesNo(); //ok
	sc.SupportAttachedOrdersForTrading = SupportAttachedOrdersForTrading.GetYesNo(); //ok
	sc.CancelAllOrdersOnEntriesAndReversals = CancelAllOrdersOnEntriesAndReversals.GetYesNo(); //ok;
	sc.CancelAllWorkingOrdersOnExit = CancelAllWorkingOrdersOnExit.GetYesNo(); //ok
	sc.AllowOnlyOneTradePerBar = AllowOnlyOneTradePerBar.GetYesNo(); //ok
	sc.SupportAttachedOrdersForTrading = SupportAttachedOrdersForTrading.GetYesNo();//ok
	sc.MaintainTradeStatisticsAndTradesData = true;

	//-------------DEFINOVANI POKYNU---------//

	//LONG
	//specifikace dynamicky pocitanych oco vazeb pro long
	s_SCNewOrder PrikazLongOCOVar;
	PrikazLongOCOVar.OrderQuantity = 2;
	PrikazLongOCOVar.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_long = (sc.High[sc.Index - 1] - sc.Low[sc.Index - 3]) / 2 / sc.TickSize; //dynamicky pocitany sl a pt pro long
	PrikazLongOCOVar.Target1Offset = sl_pt_long * sc.TickSize;
	PrikazLongOCOVar.Target2Offset = (sl_pt_long * sc.TickSize) * 3;
	PrikazLongOCOVar.StopAllOffset = sl_pt_long * sc.TickSize;
	//specifikace fixne nastavitelnych oco vazeb pro long
	s_SCNewOrder PrikazLongOCOFix;
	PrikazLongOCOFix.OrderQuantity = 2;
	PrikazLongOCOFix.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazLongOCOFix.Target1Offset = OCOFixPT1.GetInt() * sc.TickSize;
	PrikazLongOCOFix.Target2Offset = OCOFixPT2.GetInt() * sc.TickSize;
	PrikazLongOCOFix.StopAllOffset = OCOFixSL.GetInt() * sc.TickSize;
		
	//SHORT
	//specifikace dynamicky pocitanych oco vazeb pro short
	s_SCNewOrder PrikazShortOCOVar;
	PrikazShortOCOVar.OrderQuantity = 2;
	PrikazShortOCOVar.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_short = abs(sc.Low[sc.Index - 1] - sc.High[sc.Index - 3]) / 2 / sc.TickSize; //dynamicky pocitany sl a pt pro short
	PrikazShortOCOVar.Target1Offset = sl_pt_short * sc.TickSize;
	PrikazShortOCOVar.Target2Offset = (sl_pt_short * sc.TickSize) * 3;
	PrikazShortOCOVar.StopAllOffset = sl_pt_short * sc.TickSize;
	//specifikace fixne nastavitelnych oco vazeb pro short
	s_SCNewOrder PrikazShortOCOFix;
	PrikazShortOCOFix.OrderQuantity = 2;
	PrikazShortOCOFix.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazShortOCOFix.Target1Offset = OCOFixPT1.GetInt() * sc.TickSize;
	PrikazShortOCOFix.Target2Offset = OCOFixPT2.GetInt() * sc.TickSize;
	PrikazShortOCOFix.StopAllOffset = OCOFixSL.GetInt() * sc.TickSize;
	

	//promenne pro range tri baru RUSTOVYCH abych je mohl porovnat 
	float Range3Long = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2Long = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1Long = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);
	//promenne pro range tri baru KLESAJICICH abych je mohl porovnat 
	float Range3Short = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2Short = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1Short = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

	
	//-----DEFINOVANI VSTUPNI PODMINKY-----//

	//jsme li v obchodnich hodinach
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime())
	{
		//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
		if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že bary musi byt zhruba stejne dlouhé 
			if ((Range2Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
				(Range2Long >(Range3Long - odchylka.GetInt()*sc.TickSize)) &&
				(Range1Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
				(Range1Long >(Range3Long - odchylka.GetInt()*sc.TickSize)))
			{
				//kazdý low nasledujiciho bar se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
				if (sc.Low[sc.Index - 1] <= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2])) + tolerance.GetInt()*sc.TickSize  &&
					sc.Low[sc.Index - 2] <= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) + tolerance.GetInt()*sc.TickSize)
				{
					//a zaroven ten poslední musi uzavrit ve sve horni polovine
					if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						//vybarvi prislusne bary modre
						ConsolidationBarLong[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBarLong[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBarLong[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3. z nich udelej carku
						EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

						//-----OTEVRENI POZICE LONG-----//
						//kdyz se cena dotkne midu predchoziho baru vem long a zadej variabilni parametry pro sl a tg
						if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOVar);
						}
						//kdyz se cena dotkne midu predchoziho baru vem long a zadej fixni parametry pro sl a tg
						else if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOFix.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOFix);
						}
						//kdyz uzavre treti bar vem long a zadej variabilni  parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index-1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOVar.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOVar);
						}
						//kdyz uzavre treti bar vem long a zadej fixni parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOFix.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOFix);
						}
					}
				}
			}
		}

		//vytvarim sekvenci 3 baru ktery klesaji, kazdy ma nizsi highlow nez predchazejici 
		else if (((sc.High[sc.Index - 1] < sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] < sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] < sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] < sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že ty klesajici bary musi byt zhruba stejne dlouhé 
			if ((Range2Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
				(Range2Short >(Range3Short - odchylka.GetInt()*sc.TickSize)) &&
				(Range1Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
				(Range1Short >(Range3Short - odchylka.GetInt()*sc.TickSize)))
			{
				//dalsi pominka je, ze kazdy high nasledujiciho baru se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
				if (sc.High[sc.Index - 1] >= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]) - tolerance.GetInt()*sc.TickSize) &&
					sc.High[sc.Index - 2] >= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) - tolerance.GetInt()*sc.TickSize)
				{
					//a zaroven ten poslední musi uzavrit ve sve spodni polovine
					if (sc.Close[sc.Index - 1] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						//vybarvi prislusne bary zlute
						ConsolidationBarShort[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBarShort[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBarShort[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3. z nich udelej carku
						EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

						//-----OTEVRENI POZICE SHORT-----//
						//kdyz se cena dotkne midu predchoziho baru vem short a zadej variabilni parametry pro sl a tg
						if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOVar);
						}
						//kdyz se cena dotkne midu predchoziho baru vem short a zadej fixni parametry pro sl a tg
						else if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOFix.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOFix);
						}
						//kdyz uzavre treti bar, vem short a zadej variabilni parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOVar.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOVar);
						}
						//kdyz uzavre treti bar, vem short a zadej fixni parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOFix.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOFix);
						}
					}
				}
			}
		}
	}						 

	
							//---------RESENI VYSTUPU NA KONCI DNE----------
		s_SCPositionData PositionData;
		sc.GetTradePosition(PositionData);
		if (PositionData.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= konec_rth.GetTime())
	{
		sc.FlattenAndCancelAllOrders();
	}
}

SCSFExport scsf_TrendPattern_Absolutni_05(SCStudyInterfaceRef sc)
{

	SCSubgraphRef EntryPoint = sc.Subgraph[0];
	SCSubgraphRef ConsolidationBarLong = sc.Subgraph[3];
	SCSubgraphRef ConsolidationBarShort = sc.Subgraph[4];

	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[10];
	SCSubgraphRef BuyExitSubgraph = sc.Subgraph[11];

	SCInputRef zacatek_rth = sc.Input[1];
	SCInputRef konec_rth = sc.Input[2];
	SCInputRef tolerance = sc.Input[3];
	SCInputRef odchylka = sc.Input[4];

	SCInputRef TypVystupuOCOVar = sc.Input[10];
	SCInputRef TypVystupuOCOFix = sc.Input[11];

	SCInputRef OCOFixSL = sc.Input[20];
	SCInputRef OCOFixPT1 = sc.Input[21];
	SCInputRef OCOFixPT2 = sc.Input[22];

	SCInputRef MaximumPositionAllowed = sc.Input[30];
	SCInputRef PositionQuantity = sc.Input[31];
	SCInputRef AllowEntryWithWorkingOrders = sc.Input[32];
	SCInputRef SupportReversals = sc.Input[33];
	SCInputRef SendOrdersToTradeService = sc.Input[34];
	SCInputRef AllowMultipleEntriesInSameDirection = sc.Input[35];
	SCInputRef AllowOppositeEntryWithOpposingPositionOrOrders = sc.Input[36];
	SCInputRef CancelAllOrdersOnEntriesAndReversals = sc.Input[38];
	SCInputRef CancelAllWorkingOrdersOnExit = sc.Input[39];
	SCInputRef AllowOnlyOneTradePerBar = sc.Input[40];
	SCInputRef SupportAttachedOrdersForTrading = sc.Input[41];

	SCInputRef EvaluateOnBarCloseOnly = sc.Input[50];

	if (sc.SetDefaults)
	{
		sc.GraphName = "Trend pattern - Absolutni (verze 5 - po zasazeni 1. tg posouva na be)";
		sc.StudyDescription = "vstupy do trendu na prekryvajici se barech (+ pak zapojim volume/deltu nebo market ordery)";
		sc.GraphRegion = 0;

		EntryPoint.Name = "Misto vstupu";
		EntryPoint.PrimaryColor = RGB(250, 250, 250);
		EntryPoint.DrawStyle = DRAWSTYLE_DASH;
		EntryPoint.LineWidth = 5;

		BuyEntrySubgraph.Name = "Buy Entry";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
		BuyEntrySubgraph.DrawZeros = false;

		ConsolidationBarLong.Name = "Bar v konsolidaci";
		ConsolidationBarLong.PrimaryColor = RGB(128, 128, 255);
		ConsolidationBarLong.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBarLong.LineWidth = 2;

		ConsolidationBarShort.Name = "Bar v konsolidaci";
		ConsolidationBarShort.PrimaryColor = RGB(255, 255, 0);
		ConsolidationBarShort.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBarShort.LineWidth = 2;

		//nastaveni casu
		zacatek_rth.Name = "Trade From:";
		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));

		konec_rth.Name = "Trade Till:";
		konec_rth.SetTime(HMS_TIME(15, 10, 00));

		//nastaveni objemu 
		PositionQuantity.Name = "Position Quantity";
		PositionQuantity.SetInt(2);

		MaximumPositionAllowed.Name = "Maximum Position Allowed";
		MaximumPositionAllowed.SetInt(6);

		//nastaveni boolu pro trading
		AllowEntryWithWorkingOrders.Name = "Allow Entry With Working Orders (Yes/No)?";
		AllowEntryWithWorkingOrders.SetYesNo(false);

		SupportReversals.Name = "Support Reversals?";
		SupportReversals.SetYesNo(false);

		SendOrdersToTradeService.Name = "Send Orders To Trade Service?";
		SendOrdersToTradeService.SetYesNo(false);

		AllowMultipleEntriesInSameDirection.Name = "Allow Multiple Entries In Same Direction";
		AllowMultipleEntriesInSameDirection.SetYesNo(false);

		AllowOppositeEntryWithOpposingPositionOrOrders.Name = "Allow Opposite Entry With Opposing Position Or Orders";
		AllowOppositeEntryWithOpposingPositionOrOrders.SetYesNo(false);

		CancelAllOrdersOnEntriesAndReversals.Name = "Cancel All Orders On Entries And Reversals";
		CancelAllOrdersOnEntriesAndReversals.SetYesNo(false);

		CancelAllWorkingOrdersOnExit.Name = "Cancel All Working Orders On Exit";
		CancelAllWorkingOrdersOnExit.SetYesNo(true);

		AllowOnlyOneTradePerBar.Name = "Allow Only One Trade Per Bar";
		AllowOnlyOneTradePerBar.SetYesNo(true);

		SupportAttachedOrdersForTrading.Name = "Support Attached Orders For Trading";
		SupportAttachedOrdersForTrading.SetYesNo(true);

		//nastavitelnme tolerance
		odchylka.Name = "Variance Of The Bars´ Range (in ticks)";
		odchylka.SetInt(5);

		tolerance.Name = "Tolerance For The Previous Bars´ Mid Touch (in ticks)";
		tolerance.SetInt(0);

		//nastavitelne vstupy
		EvaluateOnBarCloseOnly.Name = "Evaluate on Bar Close Only";
		EvaluateOnBarCloseOnly.SetYesNo(true);

		//nastavitelne vystupy
		TypVystupuOCOVar.Name = "Exit type: OCO Range Based Variable";
		TypVystupuOCOVar.SetYesNo(true);

		TypVystupuOCOFix.Name = "Exit type: OCO Fixed";
		TypVystupuOCOFix.SetYesNo(false);

		OCOFixSL.Name = "Set SL for OCO Fixed (in ticks)";
		OCOFixSL.SetInt(15);

		OCOFixPT1.Name = "Set 1. PT for OCO Fixed (in ticks)";
		OCOFixPT1.SetInt(15);

		OCOFixPT2.Name = "Set 2. PT for OCO Fixed (in ticks)";
		OCOFixPT2.SetInt(40);

		sc.AutoLoop = 1;
		sc.FreeDLL = 1;

		return;
	}

	sc.MaximumPositionAllowed = MaximumPositionAllowed.GetInt(); //ok
	sc.AllowEntryWithWorkingOrders = AllowEntryWithWorkingOrders.GetInt(); //ok
	sc.SupportReversals = SupportReversals.GetYesNo(); //ok
	sc.SendOrdersToTradeService = SendOrdersToTradeService.GetYesNo(); //ok
	sc.AllowMultipleEntriesInSameDirection = AllowMultipleEntriesInSameDirection.GetYesNo(); //ok
	sc.AllowOppositeEntryWithOpposingPositionOrOrders = AllowOppositeEntryWithOpposingPositionOrOrders.GetYesNo(); //ok
	sc.SupportAttachedOrdersForTrading = SupportAttachedOrdersForTrading.GetYesNo(); //ok
	sc.CancelAllOrdersOnEntriesAndReversals = CancelAllOrdersOnEntriesAndReversals.GetYesNo(); //ok;
	sc.CancelAllWorkingOrdersOnExit = CancelAllWorkingOrdersOnExit.GetYesNo(); //ok
	sc.AllowOnlyOneTradePerBar = AllowOnlyOneTradePerBar.GetYesNo(); //ok
	sc.SupportAttachedOrdersForTrading = SupportAttachedOrdersForTrading.GetYesNo();//ok
	sc.MaintainTradeStatisticsAndTradesData = true;

	//-------------DEFINOVANI POKYNU---------//

	//LONG
	//specifikace dynamicky pocitanych oco vazeb pro long
	s_SCNewOrder PrikazLongOCOVar;
	PrikazLongOCOVar.OrderQuantity = 2;
	PrikazLongOCOVar.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_long = (sc.High[sc.Index - 1] - sc.Low[sc.Index - 3]) / 2 / sc.TickSize; //dynamicky pocitany sl a pt pro long
	PrikazLongOCOVar.Target1Offset = sl_pt_long * sc.TickSize;
	PrikazLongOCOVar.Target2Offset = (sl_pt_long * sc.TickSize) * 3;
	PrikazLongOCOVar.StopAllOffset = sl_pt_long * sc.TickSize;
	//po zasazeni 1.targetu z variabilnich oco posun sl na be
	PrikazLongOCOVar.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OCO_GROUP_TRIGGERED;
	PrikazLongOCOVar.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 0;
	//specifikace fixne nastavitelnych oco vazeb pro long
	s_SCNewOrder PrikazLongOCOFix;
	PrikazLongOCOFix.OrderQuantity = 2;
	PrikazLongOCOFix.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazLongOCOFix.Target1Offset = OCOFixPT1.GetInt() * sc.TickSize;
	PrikazLongOCOFix.Target2Offset = OCOFixPT2.GetInt() * sc.TickSize;
	PrikazLongOCOFix.StopAllOffset = OCOFixSL.GetInt() * sc.TickSize;
	//po zasazeni 1.targetu z fixnich oco posun sl na be
	PrikazLongOCOFix.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OCO_GROUP_TRIGGERED;
	PrikazLongOCOFix.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 0;

	//SHORT
	//specifikace dynamicky pocitanych oco vazeb pro short
	s_SCNewOrder PrikazShortOCOVar;
	PrikazShortOCOVar.OrderQuantity = 2;
	PrikazShortOCOVar.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_short = abs(sc.Low[sc.Index - 1] - sc.High[sc.Index - 3]) / 2 / sc.TickSize; //dynamicky pocitany sl a pt pro short
	PrikazShortOCOVar.Target1Offset = sl_pt_short * sc.TickSize;
	PrikazShortOCOVar.Target2Offset = (sl_pt_short * sc.TickSize) * 3;
	PrikazShortOCOVar.StopAllOffset = sl_pt_short * sc.TickSize;
	//po zasazeni 1.targetu z variabilnich oco posun sl na be
	PrikazShortOCOVar.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OCO_GROUP_TRIGGERED;
	PrikazShortOCOVar.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 0;
	//specifikace fixne nastavitelnych oco vazeb pro short
	s_SCNewOrder PrikazShortOCOFix;
	PrikazShortOCOFix.OrderQuantity = 2;
	PrikazShortOCOFix.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazShortOCOFix.Target1Offset = OCOFixPT1.GetInt() * sc.TickSize;
	PrikazShortOCOFix.Target2Offset = OCOFixPT2.GetInt() * sc.TickSize;
	PrikazShortOCOFix.StopAllOffset = OCOFixSL.GetInt() * sc.TickSize;
	//po zasazeni 1.targetu z fixnich oco posun sl na be
	PrikazShortOCOFix.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OCO_GROUP_TRIGGERED;
	PrikazShortOCOFix.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 0;

	//promenne pro range tri baru RUSTOVYCH abych je mohl porovnat 
	float Range3Long = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2Long = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1Long = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);
	//promenne pro range tri baru KLESAJICICH abych je mohl porovnat 
	float Range3Short = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2Short = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1Short = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);


	//-----DEFINOVANI VSTUPNI PODMINKY-----//

	//jsme li v obchodnich hodinach
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime())
	{
		//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
		if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že bary musi byt zhruba stejne dlouhé 
			if ((Range2Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
				(Range2Long >(Range3Long - odchylka.GetInt()*sc.TickSize)) &&
				(Range1Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
				(Range1Long >(Range3Long - odchylka.GetInt()*sc.TickSize)))
			{
				//kazdý low nasledujiciho bar se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
				if (sc.Low[sc.Index - 1] <= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2])) + tolerance.GetInt()*sc.TickSize  &&
					sc.Low[sc.Index - 2] <= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) + tolerance.GetInt()*sc.TickSize)
				{
					//a zaroven ten poslední musi uzavrit ve sve horni polovine
					if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						//vybarvi prislusne bary modre
						ConsolidationBarLong[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBarLong[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBarLong[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3. z nich udelej carku
						EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

						//-----OTEVRENI POZICE LONG-----//
						//kdyz se cena dotkne midu predchoziho baru vem long a zadej variabilni parametry pro sl a tg
						if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOVar);
						}
						//kdyz se cena dotkne midu predchoziho baru vem long a zadej fixni parametry pro sl a tg
						else if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOFix.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOFix);
						}
						//kdyz uzavre treti bar vem long a zadej variabilni  parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOVar.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOVar);
						}
						//kdyz uzavre treti bar vem long a zadej fixni parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOFix.GetYesNo())
						{
							sc.BuyEntry(PrikazLongOCOFix);
						}
					}
				}
			}
		}

		//vytvarim sekvenci 3 baru ktery klesaji, kazdy ma nizsi highlow nez predchazejici 
		else if (((sc.High[sc.Index - 1] < sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] < sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] < sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] < sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že ty klesajici bary musi byt zhruba stejne dlouhé 
			if ((Range2Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
				(Range2Short >(Range3Short - odchylka.GetInt()*sc.TickSize)) &&
				(Range1Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
				(Range1Short >(Range3Short - odchylka.GetInt()*sc.TickSize)))
			{
				//dalsi pominka je, ze kazdy high nasledujiciho baru se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
				if (sc.High[sc.Index - 1] >= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]) - tolerance.GetInt()*sc.TickSize) &&
					sc.High[sc.Index - 2] >= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) - tolerance.GetInt()*sc.TickSize)
				{
					//a zaroven ten poslední musi uzavrit ve sve spodni polovine
					if (sc.Close[sc.Index - 1] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						//vybarvi prislusne bary zlute
						ConsolidationBarShort[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBarShort[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBarShort[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3. z nich udelej carku
						EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

						//-----OTEVRENI POZICE SHORT-----//
						//kdyz se cena dotkne midu predchoziho baru vem short a zadej variabilni parametry pro sl a tg
						if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOVar);
						}
						//kdyz se cena dotkne midu predchoziho baru vem short a zadej fixni parametry pro sl a tg
						else if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOFix.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOFix);
						}
						//kdyz uzavre treti bar, vem short a zadej variabilni parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOVar.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOVar);
						}
						//kdyz uzavre treti bar, vem short a zadej fixni parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOFix.GetYesNo())
						{
							sc.SellEntry(PrikazShortOCOFix);
						}
					}
				}
			}
		}
	}


	//---------RESENI VYSTUPU NA KONCI DNE----------
	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);
	if (PositionData.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= konec_rth.GetTime())
	{
		sc.FlattenAndCancelAllOrders();
	}
}

SCSFExport scsf_TrendPattern_Absolutni_06(SCStudyInterfaceRef sc)
{

	SCSubgraphRef EntryPoint = sc.Subgraph[0];
	SCSubgraphRef ConsolidationBarLong = sc.Subgraph[3];
	SCSubgraphRef ConsolidationBarShort = sc.Subgraph[4];

	SCSubgraphRef BuyEntrySubgraph = sc.Subgraph[10];
	SCSubgraphRef BuyExitSubgraph = sc.Subgraph[11];

	SCInputRef NumberOfTransferedChart = sc.Input[0];

	SCInputRef zacatek_rth = sc.Input[1];
	SCInputRef konec_rth = sc.Input[2];
	SCInputRef tolerance = sc.Input[3];
	SCInputRef odchylka = sc.Input[4];

	SCInputRef TypVystupuOCOVar = sc.Input[5];
	SCInputRef NasobekRangePT1 = sc.Input[8];
	SCInputRef NasobekRangePT2 = sc.Input[9];

	SCInputRef TypVystupuOCOFix = sc.Input[11];
	SCInputRef OCOFixSL = sc.Input[20];
	SCInputRef OCOFixPT1 = sc.Input[21];
	SCInputRef OCOFixPT2 = sc.Input[22];

	SCInputRef MaximumPositionAllowed = sc.Input[30];
	SCInputRef PositionQuantity = sc.Input[31];
	SCInputRef AllowEntryWithWorkingOrders = sc.Input[32];
	SCInputRef SupportReversals = sc.Input[33];
	SCInputRef SendOrdersToTradeService = sc.Input[34];
	SCInputRef AllowMultipleEntriesInSameDirection = sc.Input[35];
	SCInputRef AllowOppositeEntryWithOpposingPositionOrOrders = sc.Input[36];
	SCInputRef CancelAllOrdersOnEntriesAndReversals = sc.Input[38];
	SCInputRef CancelAllWorkingOrdersOnExit = sc.Input[39];
	SCInputRef AllowOnlyOneTradePerBar = sc.Input[40];
	SCInputRef SupportAttachedOrdersForTrading = sc.Input[41];
	
	SCInputRef EvaluateOnBarCloseOnly = sc.Input[50];
	SCInputRef MoveToBE = sc.Input[51];

	SCInputRef FiltrTickNyse = sc.Input[52];

	if (sc.SetDefaults)
	{
		sc.GraphName = "Trend pattern - Absolutni (6.0 - posun na BE v inputu pro kazdy typ vystupu)";
		sc.StudyDescription = "vstupy do trendu na prekryvajici se barech (+ pak zapojim volume/deltu nebo market ordery)";
		sc.GraphRegion = 0;

		NumberOfTransferedChart.Name = "Cislo prenaseneho grafu (Tick Nyse)";
		NumberOfTransferedChart.SetChartNumber(1);

		FiltrTickNyse.Name = "Filtrovat Tickem?";
		FiltrTickNyse.SetYesNo(1);

		EntryPoint.Name = "Misto vstupu";
		EntryPoint.PrimaryColor = RGB(250, 250, 250);
		EntryPoint.DrawStyle = DRAWSTYLE_DASH;
		EntryPoint.LineWidth = 5;

		BuyEntrySubgraph.Name = "Buy Entry";
		BuyEntrySubgraph.DrawStyle = DRAWSTYLE_ARROW_UP;
		BuyEntrySubgraph.PrimaryColor = RGB(0, 255, 0);
		BuyEntrySubgraph.LineWidth = 2;
		BuyEntrySubgraph.DrawZeros = false;

		ConsolidationBarLong.Name = "Bar v konsolidaci";
		ConsolidationBarLong.PrimaryColor = RGB(128, 128, 255);
		ConsolidationBarLong.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBarLong.LineWidth = 2;

		ConsolidationBarShort.Name = "Bar v konsolidaci";
		ConsolidationBarShort.PrimaryColor = RGB(255, 255, 0);
		ConsolidationBarShort.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBarShort.LineWidth = 2;

		//nastaveni casu
		zacatek_rth.Name = "Trade From:";
		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));

		konec_rth.Name = "Trade Till:";
		konec_rth.SetTime(HMS_TIME(14, 59, 00));

		//nastaveni objemu 
		PositionQuantity.Name = "Position Quantity";
		PositionQuantity.SetInt(2);

		MaximumPositionAllowed.Name = "Maximum Position Allowed";
		MaximumPositionAllowed.SetInt(6);

		//nastaveni boolu pro trading
		AllowEntryWithWorkingOrders.Name = "Allow Entry With Working Orders (Yes/No)?";
		AllowEntryWithWorkingOrders.SetYesNo(false);

		SupportReversals.Name = "Support Reversals?";
		SupportReversals.SetYesNo(false);

		SendOrdersToTradeService.Name = "Send Orders To Trade Service?";
		SendOrdersToTradeService.SetYesNo(false);

		AllowMultipleEntriesInSameDirection.Name = "Allow Multiple Entries In Same Direction";
		AllowMultipleEntriesInSameDirection.SetYesNo(false);

		AllowOppositeEntryWithOpposingPositionOrOrders.Name = "Allow Opposite Entry With Opposing Position Or Orders";
		AllowOppositeEntryWithOpposingPositionOrOrders.SetYesNo(false);

		CancelAllOrdersOnEntriesAndReversals.Name = "Cancel All Orders On Entries And Reversals";
		CancelAllOrdersOnEntriesAndReversals.SetYesNo(false);

		CancelAllWorkingOrdersOnExit.Name = "Cancel All Working Orders On Exit";
		CancelAllWorkingOrdersOnExit.SetYesNo(true);

		AllowOnlyOneTradePerBar.Name = "Allow Only One Trade Per Bar";
		AllowOnlyOneTradePerBar.SetYesNo(true);

		SupportAttachedOrdersForTrading.Name = "Support Attached Orders For Trading";
		SupportAttachedOrdersForTrading.SetYesNo(true);

		//nastavitelnme tolerance
		odchylka.Name = "Variance Of The Bars´ Range (in ticks)";
		odchylka.SetInt(5);

		tolerance.Name = "Tolerance For The Previous Bars´ Mid Touch (in ticks)";
		tolerance.SetInt(0);

		//nastavitelne vstupy
		EvaluateOnBarCloseOnly.Name = "Evaluate on Bar Close Only";
		EvaluateOnBarCloseOnly.SetYesNo(true);

		//nastavitelne vystupy
		TypVystupuOCOVar.Name = "Exit type: OCO Range Based Variable";
		TypVystupuOCOVar.SetYesNo(true);

		TypVystupuOCOFix.Name = "Exit type: OCO Fixed";
		TypVystupuOCOFix.SetYesNo(false);

		NasobekRangePT1.Name = "Range-based 1.PT (and SL) multiplicator";
		NasobekRangePT1.SetFloat(0.5);

		NasobekRangePT2.Name = "Range-based 2.PT multiplicator";
		NasobekRangePT2.SetFloat(3);

		OCOFixSL.Name = "Set SL for OCO Fixed (in ticks)";
		OCOFixSL.SetInt(15);

		OCOFixPT1.Name = "Set 1. PT for OCO Fixed (in ticks)";
		OCOFixPT1.SetInt(15);

		OCOFixPT2.Name = "Set 2. PT for OCO Fixed (in ticks)";
		OCOFixPT2.SetInt(40);

		MoveToBE.Name = "Move SL to BE after 1. PT Hit";
		MoveToBE.SetYesNo(false);

		sc.AutoLoop = 1;
		sc.FreeDLL = 1;

		return;
	}

	sc.MaximumPositionAllowed = MaximumPositionAllowed.GetInt(); //ok
	sc.AllowEntryWithWorkingOrders = AllowEntryWithWorkingOrders.GetInt(); //ok
	sc.SupportReversals = SupportReversals.GetYesNo(); //ok
	sc.SendOrdersToTradeService = SendOrdersToTradeService.GetYesNo(); //ok
	sc.AllowMultipleEntriesInSameDirection = AllowMultipleEntriesInSameDirection.GetYesNo(); //ok
	sc.AllowOppositeEntryWithOpposingPositionOrOrders = AllowOppositeEntryWithOpposingPositionOrOrders.GetYesNo(); //ok
	sc.SupportAttachedOrdersForTrading = SupportAttachedOrdersForTrading.GetYesNo(); //ok
	sc.CancelAllOrdersOnEntriesAndReversals = CancelAllOrdersOnEntriesAndReversals.GetYesNo(); //ok;
	sc.CancelAllWorkingOrdersOnExit = CancelAllWorkingOrdersOnExit.GetYesNo(); //ok
	sc.AllowOnlyOneTradePerBar = AllowOnlyOneTradePerBar.GetYesNo(); //ok
	sc.SupportAttachedOrdersForTrading = SupportAttachedOrdersForTrading.GetYesNo();//ok
	sc.MaintainTradeStatisticsAndTradesData = true;

	//-------------DEFINOVANI POKYNU---------//

	//LONG s posunem na BE
	s_SCNewOrder PrikazLongOCOVarBE;
	PrikazLongOCOVarBE.OrderQuantity = 2;
	PrikazLongOCOVarBE.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_long_BE = (sc.High[sc.Index - 1] - sc.Low[sc.Index - 3]) * NasobekRangePT1.GetFloat() / sc.TickSize; //dynamicky pocitany sl a pt pro long
	PrikazLongOCOVarBE.Target1Offset = sl_pt_long_BE * sc.TickSize;
	PrikazLongOCOVarBE.Target2Offset = (sl_pt_long_BE * sc.TickSize) * NasobekRangePT2.GetFloat();
	PrikazLongOCOVarBE.StopAllOffset = sl_pt_long_BE * sc.TickSize;
	//po zasazeni 1.targetu z variabilnich oco posun sl na be
	PrikazLongOCOVarBE.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OCO_GROUP_TRIGGERED;
	PrikazLongOCOVarBE.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 0;
	//specifikace fixne nastavitelnych oco vazeb s posunem na BE
	s_SCNewOrder PrikazLongOCOFixBE;
	PrikazLongOCOFixBE.OrderQuantity = 2;
	PrikazLongOCOFixBE.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazLongOCOFixBE.Target1Offset = OCOFixPT1.GetInt() * sc.TickSize;
	PrikazLongOCOFixBE.Target2Offset = OCOFixPT2.GetInt() * sc.TickSize;
	PrikazLongOCOFixBE.StopAllOffset = OCOFixSL.GetInt() * sc.TickSize;
	//po zasazeni 1.targetu z fixnich oco posun sl na be
	PrikazLongOCOFixBE.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OCO_GROUP_TRIGGERED;
	PrikazLongOCOFixBE.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 0;

	//LONG bez posunu na BE
	s_SCNewOrder PrikazLongOCOVar;
	PrikazLongOCOVar.OrderQuantity = 2;
	PrikazLongOCOVar.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_long = (sc.High[sc.Index - 1] - sc.Low[sc.Index - 3]) * NasobekRangePT1.GetFloat() / sc.TickSize; //dynamicky pocitany sl a pt pro long
	PrikazLongOCOVar.Target1Offset = sl_pt_long * sc.TickSize;
	PrikazLongOCOVar.Target2Offset = (sl_pt_long * sc.TickSize) * NasobekRangePT2.GetFloat();
	PrikazLongOCOVar.StopAllOffset = sl_pt_long * sc.TickSize;
	//specifikace fixne nastavitelnych oco vazeb pro long
	s_SCNewOrder PrikazLongOCOFix;
	PrikazLongOCOFix.OrderQuantity = 2;
	PrikazLongOCOFix.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazLongOCOFix.Target1Offset = OCOFixPT1.GetInt() * sc.TickSize;
	PrikazLongOCOFix.Target2Offset = OCOFixPT2.GetInt() * sc.TickSize;
	PrikazLongOCOFix.StopAllOffset = OCOFixSL.GetInt() * sc.TickSize;
	
	//SHORT s posunem na BE
	s_SCNewOrder PrikazShortOCOVarBE;
	PrikazShortOCOVarBE.OrderQuantity = 2;
	PrikazShortOCOVarBE.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_short_BE = abs(sc.Low[sc.Index - 1] - sc.High[sc.Index - 3]) * NasobekRangePT1.GetFloat() / sc.TickSize; //dynamicky pocitany sl a pt pro short
	PrikazShortOCOVarBE.Target1Offset = sl_pt_short_BE * sc.TickSize;
	PrikazShortOCOVarBE.Target2Offset = (sl_pt_short_BE * sc.TickSize) * NasobekRangePT2.GetFloat();
	PrikazShortOCOVarBE.StopAllOffset = sl_pt_short_BE * sc.TickSize;
	//po zasazeni 1.targetu z variabilnich oco posun sl na be
	PrikazShortOCOVarBE.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OCO_GROUP_TRIGGERED;
	PrikazShortOCOVarBE.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 0;
	//specifikace fixne nastavitelnych oco vazeb s posunem na BE
	s_SCNewOrder PrikazShortOCOFixBE;
	PrikazShortOCOFixBE.OrderQuantity = 2;
	PrikazShortOCOFixBE.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazShortOCOFixBE.Target1Offset = OCOFixPT1.GetInt() * sc.TickSize;
	PrikazShortOCOFixBE.Target2Offset = OCOFixPT2.GetInt() * sc.TickSize;
	PrikazShortOCOFixBE.StopAllOffset = OCOFixSL.GetInt() * sc.TickSize;
	//po zasazeni 1.targetu z fixnich oco posun sl na be
	PrikazShortOCOFixBE.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OCO_GROUP_TRIGGERED;
	PrikazShortOCOFixBE.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 0;

	//SHORT bez posunu na BE 
	s_SCNewOrder PrikazShortOCOVar;
	PrikazShortOCOVar.OrderQuantity = 2;
	PrikazShortOCOVar.OrderType = SCT_ORDERTYPE_MARKET;
	int sl_pt_short = abs(sc.Low[sc.Index - 1] - sc.High[sc.Index - 3]) * NasobekRangePT1.GetFloat() / sc.TickSize; //dynamicky pocitany sl a pt pro short
	PrikazShortOCOVar.Target1Offset = sl_pt_short * sc.TickSize;
	PrikazShortOCOVar.Target2Offset = (sl_pt_short * sc.TickSize) * NasobekRangePT2.GetFloat();
	PrikazShortOCOVar.StopAllOffset = sl_pt_short * sc.TickSize;
	//specifikace fixne nastavitelnych oco vazeb pro short
	s_SCNewOrder PrikazShortOCOFix;
	PrikazShortOCOFix.OrderQuantity = 2;
	PrikazShortOCOFix.OrderType = SCT_ORDERTYPE_MARKET;
	PrikazShortOCOFix.Target1Offset = OCOFixPT1.GetInt() * sc.TickSize;
	PrikazShortOCOFix.Target2Offset = OCOFixPT2.GetInt() * sc.TickSize;
	PrikazShortOCOFix.StopAllOffset = OCOFixSL.GetInt() * sc.TickSize;
	
	//promenne pro range tri baru RUSTOVYCH abych je mohl porovnat 
	float Range3Long = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2Long = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1Long = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);
	//promenne pro range tri baru KLESAJICICH abych je mohl porovnat 
	float Range3Short = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2Short = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1Short = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

	//-----DEFINOVANI VSTUPNI PODMINKY-----//

	SCGraphData bazicka_data_TICK_NYSE;
	sc.GetChartBaseData(NumberOfTransferedChart.GetInt(), bazicka_data_TICK_NYSE);
	SCFloatArrayRef HL_array = bazicka_data_TICK_NYSE[SC_HL];
	if (HL_array.GetArraySize() == 0) return;

	int referencni_index = sc.GetNearestMatchForDateTimeIndex(NumberOfTransferedChart.GetInt(), sc.Index);
	float NearestRefChartHigh = HL_array[referencni_index];

	//mam li zaply posun na be
	if (MoveToBE.GetYesNo())	
	{
		//a jsme li v obchodnich hodinach
		if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime())
		{
			//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
			if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
				(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
				((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
				(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3])))
			{
				//pridavam podminku, že bary musi byt zhruba stejne dlouhé 
				if ((Range2Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
					(Range2Long >(Range3Long - odchylka.GetInt()*sc.TickSize)) &&
					(Range1Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
					(Range1Long >(Range3Long - odchylka.GetInt()*sc.TickSize)))
				{
					//kazdý low nasledujiciho bar se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
					if (sc.Low[sc.Index - 1] <= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2])) + tolerance.GetInt()*sc.TickSize  &&
						sc.Low[sc.Index - 2] <= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) + tolerance.GetInt()*sc.TickSize)
					{
						//a zaroven ten poslední musi uzavrit ve sve horni polovine
						if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
						{
							//vybarvi prislusne bary modre
							ConsolidationBarLong[sc.Index - 1] = sc.Close[sc.Index - 1];
							ConsolidationBarLong[sc.Index - 2] = sc.Close[sc.Index - 2];
							ConsolidationBarLong[sc.Index - 3] = sc.Close[sc.Index - 3];
							//v polovine 3. z nich udelej carku
							EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

							//-----OTEVRENI POZICE LONG-----//
							//kdyz se cena dotkne midu predchoziho baru vem long a zadej variabilni parametry pro sl a tg
							if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
							{
								if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] > 0 && HL_array[sc.Index - 2] > 0 && HL_array[sc.Index - 1] > 0)
								{
									sc.BuyEntry(PrikazLongOCOVarBE);
								}
								else
									sc.BuyEntry(PrikazLongOCOVarBE);
								}
							//kdyz se cena dotkne midu predchoziho baru vem long a zadej fixni parametry pro sl a tg
							else if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOFix.GetYesNo())
							{
								if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] > 0 && HL_array[sc.Index - 2] > 0 && HL_array[sc.Index - 1] > 0)
								{
									sc.BuyEntry(PrikazLongOCOFixBE);
								}
								else
									sc.BuyEntry(PrikazLongOCOFixBE);
								
							}
							//kdyz uzavre treti bar vem long a zadej variabilni  parametry pro sl a tg
							else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOVar.GetYesNo())
							{
								if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] > 0 && HL_array[sc.Index - 2] > 0 && HL_array[sc.Index - 1] > 0)
								{
									sc.BuyEntry(PrikazLongOCOVarBE);
								}
								else
									sc.BuyEntry(PrikazLongOCOVarBE);
								}
							//kdyz uzavre treti bar vem long a zadej fixni parametry pro sl a tg
							else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOFix.GetYesNo())
							{
								if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] > 0 && HL_array[sc.Index - 2] > 0 && HL_array[sc.Index - 1] > 0)
								{
									sc.BuyEntry(PrikazLongOCOFixBE);
								}
								else
									sc.BuyEntry(PrikazLongOCOFixBE);
							}
						}
					}
				}
			}

			//vytvarim sekvenci 3 baru ktery klesaji, kazdy ma nizsi highlow nez predchazejici 
			else if (((sc.High[sc.Index - 1] < sc.High[sc.Index - 2]) &&
				(sc.High[sc.Index - 2] < sc.High[sc.Index - 3])) &&
				((sc.Low[sc.Index - 1] < sc.Low[sc.Index - 2]) &&
				(sc.Low[sc.Index - 2] < sc.Low[sc.Index - 3])))
			{
				//pridavam podminku, že ty klesajici bary musi byt zhruba stejne dlouhé 
				if ((Range2Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
					(Range2Short >(Range3Short - odchylka.GetInt()*sc.TickSize)) &&
					(Range1Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
					(Range1Short >(Range3Short - odchylka.GetInt()*sc.TickSize)))
				{
					//dalsi pominka je, ze kazdy high nasledujiciho baru se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
					if (sc.High[sc.Index - 1] >= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]) - tolerance.GetInt()*sc.TickSize) &&
						sc.High[sc.Index - 2] >= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) - tolerance.GetInt()*sc.TickSize)
					{
						//a zaroven ten poslední musi uzavrit ve sve spodni polovine
						if (sc.Close[sc.Index - 1] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
						{
							//vybarvi prislusne bary zlute
							ConsolidationBarShort[sc.Index - 1] = sc.Close[sc.Index - 1];
							ConsolidationBarShort[sc.Index - 2] = sc.Close[sc.Index - 2];
							ConsolidationBarShort[sc.Index - 3] = sc.Close[sc.Index - 3];
							//v polovine 3. z nich udelej carku
							EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

							//-----OTEVRENI POZICE SHORT-----//
							//kdyz se cena dotkne midu predchoziho baru vem short a zadej variabilni parametry pro sl a tg
							if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
							{ 
								if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] < 0 && HL_array[sc.Index - 2] < 0 && HL_array[sc.Index - 1] < 0)
								{
									sc.SellEntry(PrikazShortOCOVarBE);
								}
								else
									sc.SellEntry(PrikazShortOCOVarBE);
							}
							//kdyz se cena dotkne midu predchoziho baru vem short a zadej fixni parametry pro sl a tg
							else if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOFix.GetYesNo())
							{
								if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] < 0 && HL_array[sc.Index - 2] < 0 && HL_array[sc.Index - 1] < 0)
								{
									sc.SellEntry(PrikazShortOCOFixBE);
								}
								else
									sc.SellEntry(PrikazShortOCOFixBE);
							}
							//kdyz uzavre treti bar, vem short a zadej variabilni parametry pro sl a tg
							else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOVar.GetYesNo())
							{
								if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] < 0 && HL_array[sc.Index - 2] < 0 && HL_array[sc.Index - 1] < 0)
								{
									sc.SellEntry(PrikazShortOCOVarBE);
								}
								else
									sc.SellEntry(PrikazShortOCOVarBE);
							}
							//kdyz uzavre treti bar, vem short a zadej fixni parametry pro sl a tg
							else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOFix.GetYesNo())
							{
								if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] < 0 && HL_array[sc.Index - 2] < 0 && HL_array[sc.Index - 1] < 0)
								{
									sc.SellEntry(PrikazShortOCOFixBE);
								}
								else
									sc.SellEntry(PrikazShortOCOFixBE);
							}
						}
					}
				}
			}
		}

	}

	//nemam li zaply posun na be
	else {
	//a jsme li v obchodnich hodinach
if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime())
	{
		//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
		if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že bary musi byt zhruba stejne dlouhé 
			if ((Range2Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
				(Range2Long >(Range3Long - odchylka.GetInt()*sc.TickSize)) &&
				(Range1Long < (Range3Long + odchylka.GetInt()*sc.TickSize)) &&
				(Range1Long >(Range3Long - odchylka.GetInt()*sc.TickSize)))
			{
				//kazdý low nasledujiciho bar se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
				if (sc.Low[sc.Index - 1] <= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2])) + tolerance.GetInt()*sc.TickSize  &&
					sc.Low[sc.Index - 2] <= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) + tolerance.GetInt()*sc.TickSize)
				{
					//a zaroven ten poslední musi uzavrit ve sve horni polovine
					if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						//vybarvi prislusne bary modre
						ConsolidationBarLong[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBarLong[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBarLong[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3. z nich udelej carku
						EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

						//-----OTEVRENI POZICE LONG-----//
						//kdyz se cena dotkne midu predchoziho baru vem long a zadej variabilni parametry pro sl a tg
						if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
						{
							//kdyz je zapnute filtrovani tickem na ANO -- vstup jen tehdy, je li higlow average TICK NYSE (tj. to pole ktery prenasim) u vsech 3 baru kladne
							if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] > 0 && HL_array[sc.Index - 2] > 0 && HL_array[sc.Index - 1] > 0)
							{
								sc.BuyEntry(PrikazLongOCOVar);
							}
							else
								sc.BuyEntry(PrikazLongOCOVar);
						}
						//kdyz se cena dotkne midu predchoziho baru vem long a zadej fixni parametry pro sl a tg
						else if (sc.Close[sc.Index] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOFix.GetYesNo())
						{
							//kdyz je zapnute filtrovani tickem na ANO -- vstup jen tehdy, je li higlow average TICK NYSE (tj. to pole ktery prenasim) u vsech 3 baru kladne
							if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] > 0 && HL_array[sc.Index - 2] > 0 && HL_array[sc.Index - 1] > 0)
							{
								sc.BuyEntry(PrikazLongOCOFix);
							}
							else
								sc.BuyEntry(PrikazLongOCOFix);
						}
						//kdyz uzavre treti bar vem long a zadej variabilni  parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOVar.GetYesNo())
						{
							
							//kdyz je zapnute filtrovani tickem na ANO -- vstup jen tehdy, je li higlow average TICK NYSE (tj. to pole ktery prenasim) u vsech 3 baru kladne
							if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] > 0 && HL_array[sc.Index - 2] > 0 && HL_array[sc.Index - 1] > 0)
							{
								sc.BuyEntry(PrikazLongOCOVar);
							}
							else 
								sc.BuyEntry(PrikazLongOCOVar);
						}
						//kdyz uzavre treti bar vem long a zadej fixni parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOFix.GetYesNo())
						{
							//kdyz je zapnute filtrovani tickem na ANO -- vstup jen tehdy, je li higlow average TICK NYSE (tj. to pole ktery prenasim) u vsech 3 baru kladne
							if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] > 0 && HL_array[sc.Index - 2] > 0 && HL_array[sc.Index - 1] > 0)
							{
								sc.BuyEntry(PrikazLongOCOFix);
							}
							else 
								sc.BuyEntry(PrikazLongOCOFix);
						}
					}
				}
			}
		}

		//vytvarim sekvenci 3 baru ktery klesaji, kazdy ma nizsi highlow nez predchazejici 
		else if (((sc.High[sc.Index - 1] < sc.High[sc.Index - 2]) &&
			(sc.High[sc.Index - 2] < sc.High[sc.Index - 3])) &&
			((sc.Low[sc.Index - 1] < sc.Low[sc.Index - 2]) &&
			(sc.Low[sc.Index - 2] < sc.Low[sc.Index - 3])))
		{
			//pridavam podminku, že ty klesajici bary musi byt zhruba stejne dlouhé 
			if ((Range2Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
				(Range2Short >(Range3Short - odchylka.GetInt()*sc.TickSize)) &&
				(Range1Short < (Range3Short + odchylka.GetInt()*sc.TickSize)) &&
				(Range1Short >(Range3Short - odchylka.GetInt()*sc.TickSize)))
			{
				//dalsi pominka je, ze kazdy high nasledujiciho baru se musi aspon dotykat midu predchazejiciho baru (aby to byly fakt prekryvajici se bary)
				if (sc.High[sc.Index - 1] >= (sc.Low[sc.Index - 2] + 0.5*(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]) - tolerance.GetInt()*sc.TickSize) &&
					sc.High[sc.Index - 2] >= (sc.Low[sc.Index - 3] + 0.5*(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3])) - tolerance.GetInt()*sc.TickSize)
				{
					//a zaroven ten poslední musi uzavrit ve sve spodni polovine
					if (sc.Close[sc.Index - 1] <= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]))
					{
						//vybarvi prislusne bary zlute
						ConsolidationBarShort[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBarShort[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBarShort[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3. z nich udelej carku
						EntryPoint[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

						//-----OTEVRENI POZICE SHORT-----//
						//kdyz se cena dotkne midu predchoziho baru vem short a zadej variabilni parametry pro sl a tg
						if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOVar.GetYesNo())
						{
							if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] < 0 && HL_array[sc.Index - 2] < 0 && HL_array[sc.Index - 1] < 0)
							{
								sc.SellEntry(PrikazShortOCOVar);
							}
							else
								sc.SellEntry(PrikazShortOCOVar);
						}
						//kdyz se cena dotkne midu predchoziho baru vem short a zadej fixni parametry pro sl a tg
						else if (sc.Close[sc.Index] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]) && TypVystupuOCOFix.GetYesNo())
						{
							if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] < 0 && HL_array[sc.Index - 2] < 0 && HL_array[sc.Index - 1] < 0)
							{
								sc.SellEntry(PrikazShortOCOFix);
							}
							else
								sc.SellEntry(PrikazShortOCOFix);
						}
						//kdyz uzavre treti bar, vem short a zadej variabilni parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOVar.GetYesNo())
						{
							if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] < 0 && HL_array[sc.Index - 2] < 0 && HL_array[sc.Index - 1] < 0)
							{
								sc.SellEntry(PrikazShortOCOVar);
							}
							else
								sc.SellEntry(PrikazShortOCOVar);
						}
						//kdyz uzavre treti bar, vem short a zadej fixni parametry pro sl a tg
						else if (EvaluateOnBarCloseOnly.GetYesNo() && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED && TypVystupuOCOFix.GetYesNo())
						{
							if (FiltrTickNyse.GetYesNo() && HL_array[sc.Index - 3] < 0 && HL_array[sc.Index - 2] < 0 && HL_array[sc.Index - 1] < 0)
							{
								sc.SellEntry(PrikazShortOCOFix);
							}
							else
								sc.SellEntry(PrikazShortOCOFix);
						}
					}
				}
			}
		}
	}
	

	}

	//---------RESENI VYSTUPU NA KONCI DNE----------
	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);
	if (PositionData.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= konec_rth.GetTime())
	{
		sc.FlattenAndCancelAllOrders();
	}
}

//TOTO JE FUNKCNI KOD PRO FILTRACI TICKEM
SCSFExport scsf_TrendPattern_Absolutni_TestFiltrovaniTickem(SCStudyInterfaceRef sc)
{
	SCSubgraphRef EntryPointLong = sc.Subgraph[0];
	SCSubgraphRef ConsolidationBar = sc.Subgraph[3];
	SCSubgraphRef ConsolidationBarTick = sc.Subgraph[4];

	SCInputRef odchylka = sc.Input[0];
	SCInputRef zacatek_rth = sc.Input[1];
	SCInputRef konec_rth = sc.Input[2];
	SCInputRef cislo_grafu = sc.Input[3];
	SCInputRef logovani = sc.Input[4];

	if (sc.SetDefaults)
	{
		sc.GraphName = "Trend pattern - Absolutni, Test filtrovani Tickem";
		sc.StudyDescription = "vstupy do trendu na prekryvajici se barech (+ pak zapojim volume/deltu nebo market ordery)";
		sc.GraphRegion = 0;

		EntryPointLong.Name = "Misto vstupu";
		EntryPointLong.PrimaryColor = RGB(250, 250, 250);
		EntryPointLong.DrawStyle = DRAWSTYLE_DASH;
		EntryPointLong.LineWidth = 5;

		ConsolidationBar.Name = "Bar v konsolidaci";
		ConsolidationBar.PrimaryColor = RGB(128, 128, 255);
		ConsolidationBar.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBar.LineWidth = 2;

		ConsolidationBarTick.Name = "Bar v konsolidaci";
		ConsolidationBarTick.PrimaryColor = RGB(255, 182, 193);
		ConsolidationBarTick.DrawStyle = DRAWSTYLE_COLOR_BAR;
		ConsolidationBarTick.LineWidth = 2;

		logovani.Name = "Vypis do logu?";
		logovani.SetYesNo(1);

		sc.Subgraph[10].Name = "pøenesene pole";

		//nastaveni casu
		zacatek_rth.Name = "Trade From:";
		zacatek_rth.SetTime(HMS_TIME(8, 30, 0));

		konec_rth.Name = "Trade Till:";
		konec_rth.SetTime(HMS_TIME(14, 59, 00));

		sc.AutoLoop = 1; //manual
		sc.FreeDLL = 1;

		sc.AllowMultipleEntriesInSameDirection = true;
		sc.MaximumPositionAllowed = 1000;
		sc.SupportReversals = false;
		sc.SendOrdersToTradeService = false;
		sc.AllowOppositeEntryWithOpposingPositionOrOrders = true;
		sc.SupportAttachedOrdersForTrading = true;
		sc.CancelAllOrdersOnEntriesAndReversals = false;
		sc.AllowEntryWithWorkingOrders = false;
		sc.CancelAllWorkingOrdersOnExit = false;
		sc.AllowOnlyOneTradePerBar = true;
		sc.MaintainTradeStatisticsAndTradesData = true;

		odchylka.Name = "Odchylka v ticich";
		odchylka.SetInt(5);

		cislo_grafu.Name = "Cislo grafu";
		cislo_grafu.SetChartNumber(2);

		sc.CalculationPrecedence = LOW_PREC_LEVEL;
		sc.UpdateAlways = 1;

		return;
	}

	
	//double& cas_stareho_indexu = sc.GetPersistentDouble(1); //cas je double!


	//vypis do logu na zjisteni poctu indexu - funguje v replay i v realitme, nefunguje pri skrolovani
	//int& stary_index = sc.GetPersistentInt(1);
	//
	//if (logovani.GetYesNo() == 1 && stary_index != sc.ArraySize)
	//{
	//	SCString pocet_indexu;
	//	pocet_indexu.Format("[pocet_indexu: %i]", sc.ArraySize);
	//	sc.AddMessageToLog(pocet_indexu, 1);
	//	stary_index = sc.ArraySize;
	//}
	//

	//vypis do logu na zjisteni poctu indexu - funguje pri skrolovani
	int& stary_index = sc.GetPersistentInt(1);
	
	if (logovani.GetYesNo() == 1 && stary_index != sc.IndexOfLastVisibleBar)
	{
		SCString pocet_indexu;
		pocet_indexu.Format("[pocet_indexu: %i]", sc.IndexOfLastVisibleBar);
		sc.AddMessageToLog(pocet_indexu, 1);
		stary_index = sc.IndexOfLastVisibleBar;
	}
	
	//pokyn
	s_SCNewOrder Prikaz;
	Prikaz.OrderQuantity = 2;
	Prikaz.OrderType = SCT_ORDERTYPE_MARKET;

	// zakladam perzist vary pro uchovani ID pro pt a sl
	int& Target1OrderID = sc.GetPersistentInt(1);
	int& Stop1OrderID = sc.GetPersistentInt(2);

	//specifikace oco pokynu
	Prikaz.Target1Offset = 10 * sc.TickSize;
	Prikaz.Target2Offset = 30 * sc.TickSize;
	Prikaz.StopAllPrice = sc.Low[sc.Index - 1] - 10 * sc.TickSize;

	//promenne pro range tech tri baru abych je mohl porovnat 
	float Range3 = abs(sc.High[sc.Index - 3] - sc.Low[sc.Index - 3]);
	float Range2 = abs(sc.High[sc.Index - 2] - sc.Low[sc.Index - 2]);
	float Range1 = abs(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);

	SCGraphData bazicka_data_TICK_NYSE;
	sc.GetChartBaseData(cislo_grafu.GetInt(), bazicka_data_TICK_NYSE);
	SCFloatArrayRef HL_array = bazicka_data_TICK_NYSE[SC_HL];
	//if (HL_array.GetArraySize() == 0) return;

	int referencni_index =	sc.GetNearestMatchForDateTimeIndex(cislo_grafu.GetInt(), sc.Index);
	float NearestRefChartHigh = HL_array[referencni_index];

	//-----DEFINOVANI VSTUPNI PODMINKY-----//

	//jsme li v obchodnich hodinach
	if (sc.BaseDateTimeIn[sc.Index].GetTime() > zacatek_rth.GetTime() && sc.BaseDateTimeIn[sc.Index].GetTime() < konec_rth.GetTime())
		{
			//vytvarim sekvenci 3 baru ktery rostou, kazdy ma vyssi highlow nez predchazejici 
			if (((sc.High[sc.Index - 1] > sc.High[sc.Index - 2]) &&
				(sc.High[sc.Index - 2] > sc.High[sc.Index - 3])) &&
				((sc.Low[sc.Index - 1] > sc.Low[sc.Index - 2]) &&
				(sc.Low[sc.Index - 2] > sc.Low[sc.Index - 3])))
			{
				//pridavam podminku, že bary musi byt zhruba stejne dlouhé 
				if ((Range2 < (Range3 + odchylka.GetInt()*sc.TickSize)) &&
					(Range2 >(Range3 - odchylka.GetInt()*sc.TickSize)) &&
					(Range1 < (Range3 + odchylka.GetInt()*sc.TickSize)) &&
					(Range1 >(Range3 - odchylka.GetInt()*sc.TickSize)))
				{
					//a zaroven ten poslední musi uzavrit ve sve horni polovine
					if (sc.Close[sc.Index - 1] >= sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1])  && sc.GetBarHasClosedStatus(sc.Index - 1) == BHCS_BAR_HAS_CLOSED)
					{
						//vybarvi prislusne bary 
						ConsolidationBar[sc.Index - 1] = sc.Close[sc.Index - 1];
						ConsolidationBar[sc.Index - 2] = sc.Close[sc.Index - 2];
						ConsolidationBar[sc.Index - 3] = sc.Close[sc.Index - 3];
						//v polovine 3 z nich udelej carku
						EntryPointLong[sc.Index - 1] = sc.Low[sc.Index - 1] + 0.5*(sc.High[sc.Index - 1] - sc.Low[sc.Index - 1]);
						{
							//vstup jen tehdy, je li higlow average TICK NYSE (tj. to pole ktery prenasim) u vsech 3 baru kladne
							if (HL_array[sc.Index - 3] > 0 && HL_array[sc.Index - 2] > 0 && HL_array[sc.Index - 1] > 0)
							{
								sc.BuyEntry(Prikaz);	
							}
						}
					}	
				}
			}
		}

	//---------RESENI VYSTUPU NA KONCI DNE----------

	s_SCPositionData PositionData;
	sc.GetTradePosition(PositionData);
	if (PositionData.PositionQuantity != 0 && sc.BaseDateTimeIn[sc.Index].GetTime() >= konec_rth.GetTime())
	{
		sc.FlattenAndCancelAllOrders();
	}
}


/*============================================================================
This function gives examples of adding messages to the Message Log, playing
alert sounds and adding Alert Messages.
----------------------------------------------------------------------------*/
SCSFExport scsf_LogAndAlertExample(SCStudyInterfaceRef sc)
{
	// Set configuration variables

	if (sc.SetDefaults)
	{
		sc.GraphName = "Log and Alert Example";

		sc.StudyDescription = "";

		sc.GraphRegion = 0;

		sc.AutoLoop = 0;//manual looping

						//During development set this flag to 1, so the DLL can be modified. When development is completed, set it to 0 to improve performance.
		sc.FreeDLL = 1;

		return;
	}


	// Do data processing
	if (sc.GetBarHasClosedStatus(sc.UpdateStartIndex) == BHCS_BAR_HAS_NOT_CLOSED)
		return;

	// Show message to log
	sc.AddMessageToLog("Popped up Message", 1);
	sc.AddMessageToLog("Non-Popped up Message", 0);

	// Alert sounds are configured by selecting "Global Settings >> General Settings" in Sierra Chart
	sc.PlaySound(1);  // Add alert sound 1 to be played
	sc.PlaySound(50);  // Add alert sound 50 to be played

					   // Add an Alert message to SierraChart Alert Log
	sc.AddAlertLine("Condition is TRUE");

	// Adding an alert line using an SCString. The Alert log will also be opened.
	SCString AlertMessage("Alert Message");
	sc.AddAlertLine(AlertMessage, 1);

	// Playing a sound and adding an Alert Line at the same time.
	sc.PlaySound(2, AlertMessage, 1); // Alert Log will open

	sc.PlaySound(3, "Alert Message 2", 0); // Alert Log will not open

										   // Can also Play Sound from a text file, for example:
	sc.PlaySound("C:\\Windows\\beep.wav", 2);

	sc.AddAlertLineWithDateTime("Alert message with date-time", 0, sc.BaseDateTimeIn[sc.UpdateStartIndex]);
}