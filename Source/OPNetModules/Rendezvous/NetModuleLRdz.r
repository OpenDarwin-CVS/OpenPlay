#include <MacTypes.r>
#include <Dialogs.r>


resource 'DITL' (2000, "IP Host Selector", purgeable)
{
	{
	/* [1] */
		{ 10, 10, 26, 420 },
		StaticText
		{
			disabled,
			"Enter the host name and port of the game you wish to join:"
		},
	/* [2] */
		{ 40, 27, 56, 102 },
		StaticText
		{
			disabled,
			"Host Name:"
		},
	/* [3] */
		{ 40, 106, 56, 366 },
		EditText
		{
			enabled,
			""
		},
	/* [4] */
		{ 70, 36, 86, 103 },
		StaticText
		{
			disabled,
			"Host Port:"
		},
	/* [5] */
		{ 70, 106, 86, 176 },
		EditText
		{
			enabled,
			""
		}
	}
};

resource 'dlgx' (2000) {
	versionZero {
		15
	}
};

resource 'DLOG' (2000, purgeable)
{
	{ 60, 40, 270, 430 },
	kWindowDocumentProc,
	visible,
	goAway,
	0x0,
	2000,
	"IP Host Selector",
	alertPositionMainScreen
};

resource 'STR#' (2000, "Info", purgeable)
{
	{
		"Rendezvous",
		"2002 Freeverse, Inc."
	}
};
