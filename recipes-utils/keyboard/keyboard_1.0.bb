DESCRIPTION = "GY-91 test program"
LICENSE = "CLOSED"
PR = "r0"

SRC_URI = " \
	file://fr-latin9.map.gz \
"

S = "${WORKDIR}"

do_install() {
        install -d ${D}/
        install -m 0744 ${S}/fr-latin9.map.gz ${D}/
}
