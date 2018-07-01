DESCRIPTION = "Setup a default (french) keyboard-layout for the console only"
LICENSE = "CLOSED"
PR = "r0"

inherit systemd

SRC_URI = "file://kbdlayout.service \
	file://fr_CH-latin1.bmap \
          "

do_install () {
	install -d ${D}/${sysconfdir}
	install -m 0755 ${WORKDIR}/fr_CH-latin1.bmap ${D}/${sysconfdir}/keymap.map

	install -d ${D}/${base_libdir}/systemd/system
	install -m 0644 ${WORKDIR}/kbdlayout.service ${D}/${base_libdir}/systemd/system/
}

NATIVE_SYSTEMD_SUPPORT = "1"
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "kbdlayout.service"

FILES_${PN} += "${base_libdir}/systemd"
