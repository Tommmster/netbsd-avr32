/*
 * MACHINE GENERATED: DO NOT EDIT
 *
 * ioconf.c, from "MINIMAL"
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/mount.h>

static const struct cfiattrdata gpibdevcf_iattrdata = {
	"gpibdev", 1,
	{
		{"address", "-1", -1},
	}
};
static const struct cfiattrdata acpibuscf_iattrdata = {
	"acpibus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata caccf_iattrdata = {
	"cac", 1,
	{
		{"unit", "-1", -1},
	}
};
static const struct cfiattrdata spicf_iattrdata = {
	"spi", 1,
	{
		{"slave", "NULL", 0},
	}
};
static const struct cfiattrdata radiodevcf_iattrdata = {
	"radiodev", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata mlxcf_iattrdata = {
	"mlx", 1,
	{
		{"unit", "-1", -1},
	}
};
static const struct cfiattrdata scsibuscf_iattrdata = {
	"scsibus", 2,
	{
		{"target", "-1", -1},
		{"lun", "-1", -1},
	}
};
static const struct cfiattrdata videobuscf_iattrdata = {
	"videobus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata isabuscf_iattrdata = {
	"isabus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata i2cbuscf_iattrdata = {
	"i2cbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata ata_hlcf_iattrdata = {
	"ata_hl", 1,
	{
		{"drive", "-1", -1},
	}
};
static const struct cfiattrdata mainbuscf_iattrdata = {
	"mainbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata zsccf_iattrdata = {
	"zsc", 1,
	{
		{"channel", "-1", -1},
	}
};
static const struct cfiattrdata ibuscf_iattrdata = {
	"ibus", 1,
	{
		{"addr", "-1", -1},
	}
};
static const struct cfiattrdata depcacf_iattrdata = {
	"depca", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata ppbuscf_iattrdata = {
	"ppbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata eisabuscf_iattrdata = {
	"eisabus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata atapicf_iattrdata = {
	"atapi", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata tccf_iattrdata = {
	"tc", 2,
	{
		{"slot", "-1", -1},
		{"offset", "-1", -1},
	}
};
static const struct cfiattrdata dtcf_iattrdata = {
	"dt", 1,
	{
		{"addr", "-1", -1},
	}
};
static const struct cfiattrdata atapibuscf_iattrdata = {
	"atapibus", 1,
	{
		{"drive", "-1", -1},
	}
};
static const struct cfiattrdata dzcf_iattrdata = {
	"dz", 1,
	{
		{"line", "-1", -1},
	}
};
static const struct cfiattrdata at32buscf_iattrdata = {
	"at32bus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata onewirebuscf_iattrdata = {
	"onewirebus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata tcdscf_iattrdata = {
	"tcds", 1,
	{
		{"chip", "-1", -1},
	}
};
static const struct cfiattrdata gpiocf_iattrdata = {
	"gpio", 2,
	{
		{"offset", "NULL", 0},
		{"mask", "NULL", 0},
	}
};
static const struct cfiattrdata cbbuscf_iattrdata = {
	"cbbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata gpiobuscf_iattrdata = {
	"gpiobus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata drmcf_iattrdata = {
	"drm", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata pckbportcf_iattrdata = {
	"pckbport", 1,
	{
		{"slot", "-1", -1},
	}
};
static const struct cfiattrdata irbuscf_iattrdata = {
	"irbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata aaccf_iattrdata = {
	"aac", 1,
	{
		{"unit", "-1", -1},
	}
};
static const struct cfiattrdata pcibuscf_iattrdata = {
	"pcibus", 1,
	{
		{"bus", "-1", -1},
	}
};
static const struct cfiattrdata upccf_iattrdata = {
	"upc", 1,
	{
		{"offset", "-1", -1},
	}
};
static const struct cfiattrdata iiccf_iattrdata = {
	"iic", 2,
	{
		{"addr", "-1", -1},
		{"size", "-1", -1},
	}
};
static const struct cfiattrdata onewirecf_iattrdata = {
	"onewire", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata mcabuscf_iattrdata = {
	"mcabus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata wsdisplaydevcf_iattrdata = {
	"wsdisplaydev", 1,
	{
		{"kbdmux", "1", 1},
	}
};
static const struct cfiattrdata miicf_iattrdata = {
	"mii", 1,
	{
		{"phy", "-1", -1},
	}
};
static const struct cfiattrdata cpcbuscf_iattrdata = {
	"cpcbus", 2,
	{
		{"addr", "NULL", 0},
		{"irq", "-1", -1},
	}
};
static const struct cfiattrdata parportcf_iattrdata = {
	"parport", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata dbcoolcf_iattrdata = {
	"dbcool", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata wskbddevcf_iattrdata = {
	"wskbddev", 2,
	{
		{"console", "-1", -1},
		{"mux", "1", 1},
	}
};
static const struct cfiattrdata audiobuscf_iattrdata = {
	"audiobus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata btbuscf_iattrdata = {
	"btbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata midibuscf_iattrdata = {
	"midibus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata vmebuscf_iattrdata = {
	"vmebus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata wsemuldisplaydevcf_iattrdata = {
	"wsemuldisplaydev", 2,
	{
		{"console", "-1", -1},
		{"kbdmux", "1", 1},
	}
};
static const struct cfiattrdata ioasiccf_iattrdata = {
	"ioasic", 1,
	{
		{"offset", "-1", -1},
	}
};
static const struct cfiattrdata icpcf_iattrdata = {
	"icp", 1,
	{
		{"unit", "-1", -1},
	}
};
static const struct cfiattrdata sdmmcbuscf_iattrdata = {
	"sdmmcbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata comcf_iattrdata = {
	"com", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata spiflashbuscf_iattrdata = {
	"spiflashbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata fwbuscf_iattrdata = {
	"fwbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata pcmciaslotcf_iattrdata = {
	"pcmciaslot", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata usbuscf_iattrdata = {
	"usbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata wsmousedevcf_iattrdata = {
	"wsmousedev", 1,
	{
		{"mux", "0", 0},
	}
};
static const struct cfiattrdata scsicf_iattrdata = {
	"scsi", 1,
	{
		{"channel", "-1", -1},
	}
};
static const struct cfiattrdata atacf_iattrdata = {
	"ata", 1,
	{
		{"channel", "-1", -1},
	}
};
static const struct cfiattrdata spibuscf_iattrdata = {
	"spibus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata pcmciabuscf_iattrdata = {
	"pcmciabus", 2,
	{
		{"controller", "-1", -1},
		{"socket", "-1", -1},
	}
};
static const struct cfiattrdata at32pmcf_iattrdata = {
	"at32pm", 0,
	{
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata at32intccf_iattrdata = {
	"at32intc", 0,
	{
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata at32usartcf_iattrdata = {
	"at32usart", 0,
	{
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata at32clockcf_iattrdata = {
	"at32clock", 0,
	{
		{ NULL, NULL, 0 },
	}
};

static const struct cfiattrdata * const mainbus_attrs[] = { &mainbuscf_iattrdata, NULL };
static const struct cfiattrdata * const at32bus_attrs[] = { &at32buscf_iattrdata, NULL };

CFDRIVER_DECL(mainbus, DV_DULL, mainbus_attrs);
CFDRIVER_DECL(cpu, DV_DULL, NULL);
CFDRIVER_DECL(md, DV_DISK, NULL);
CFDRIVER_DECL(at32bus, DV_BUS, at32bus_attrs);
CFDRIVER_DECL(at32pm, DV_DULL, NULL);
CFDRIVER_DECL(at32intc, DV_DULL, NULL);
CFDRIVER_DECL(at32usart, DV_DULL, NULL);
CFDRIVER_DECL(at32clock, DV_DULL, NULL);

struct cfdriver * const cfdriver_list_initial[] = {
	&md_cd,
	&mainbus_cd,
	&cpu_cd,
	&at32bus_cd,
	&at32pm_cd,
	&at32intc_cd,
	&at32usart_cd,
	&at32clock_cd,
	NULL
};

extern struct cfattach mainbus_ca;
extern struct cfattach cpu_ca;
extern struct cfattach at32bus_ca;
extern struct cfattach at32pm_ca;
extern struct cfattach at32intc_ca;
extern struct cfattach at32usart_ca;
extern struct cfattach at32clock_ca;
extern struct cfattach md_ca;

static struct cfattach * const mainbus_cfattachinit[] = {
	&mainbus_ca, NULL
};
static struct cfattach * const cpu_cfattachinit[] = {
	&cpu_ca, NULL
};
static struct cfattach * const at32bus_cfattachinit[] = {
        &at32bus_ca, NULL
};
static struct cfattach * const at32pm_cfattachinit[] = {
        &at32pm_ca, NULL
};
static struct cfattach * const at32intc_cfattachinit[] = {
        &at32intc_ca, NULL
};
static struct cfattach * const at32usart_cfattachinit[] = {
        &at32usart_ca, NULL
};
static struct cfattach * const at32clock_cfattachinit[] = {
        &at32clock_ca, NULL
};

const struct cfattachinit cfattachinit[] = {
	/* driver           attachment    unit state loc   flags pspec */
	{ "mainbus", mainbus_cfattachinit },
	{ "cpu", cpu_cfattachinit },
	{ "at32bus", at32bus_cfattachinit },
	{ "at32pm", at32pm_cfattachinit },
	{ "at32intc", at32intc_cfattachinit },
	{ "at32usart", at32usart_cfattachinit },
	{ "at32clock", at32clock_cfattachinit },
	{ NULL, NULL }
};

/* locators */
static int loc[1] = { -1 };

static const struct cfparent pspec0 = {
	"mainbus", "mainbus", 0
};
static const struct cfparent pspec1 = {
	"at32bus", "at32bus", DVUNIT_ANY,
};

#define NORM FSTATE_NOTFOUND
#define STAR FSTATE_STAR

struct cfdata cfdata[] = {
    /* driver           attachment    unit state loc   flags pspec */
/*  0: mainbus0 at root */
    {"mainbus",		"mainbus",	 0, NORM,     loc,      0, NULL},
/*  1: cpu* at mainbus0 */
    {"cpu",		"cpu",		 0, STAR,     loc,      0, &pspec0},
/*  2: at32bus* at mainbus0 */
    {"at32bus",		"at32bus",	 0, STAR,     loc,      0, &pspec0},
/*  3: at32pm* at at32bus? */
    {"at32pm",		"at32pm",	 0, STAR,     loc,      0, &pspec1},
/*  4: at32intc* at at32bus? */
    {"at32intc",	"at32intc",	 0, STAR,     loc,      0, &pspec1},
/*  5: at32usart* at at32bus? */
    {"at32usart",	"at32usart",	 0, STAR,     loc,      0, &pspec1},
/*  6: at32clock* at at32bus? */
    {"at32clock",	"at32clock",	 0, STAR,     loc,      0, &pspec1},
    {NULL,		NULL,		 0, 0,    NULL,      0, NULL}
};

const short cfroots[] = {
	 0 /* mainbus0 */,
	-1
};

/* pseudo-devices */
void cpuctlattach(int);
void mdattach(int);

struct pdevinit pdevinit[] = {
	{ cpuctlattach, 1 },
	{ mdattach, 1 },
	{ 0, 0 }
};
