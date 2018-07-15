DESCRIPTION = "Setup a default (french) keyboard-layout for the console only"
LICENSE = "CLOSED"
PR = "r0"

inherit update-rc.d

RDEPENDS_${PN} = "initscripts"

INITSCRIPT_PARAMS = "start 02 2 3 4 5 . stop 01 0 1 6 ."
INITSCRIPT_NAME = "load_fr_keyboard.sh"

CONFFILES_${PN} += "${D}${sysconfdir}/init.d/load_fr_keyboard.sh"

SRC_URI = "file://load_fr_keyboard.sh \
          "

do_install () {
	install -d ${D}${sysconfdir}/init.d/
	install -m 0755 ${WORKDIR}/load_fr_keyboard.sh ${D}${sysconfdir}/init.d/
}

FILES_${PN} += "/etc/init.d/"
