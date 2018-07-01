DESCRIPTION = "PWM test program"
LICENSE = "CLOSED"
PR = "r0"

SRC_URI = " \
	file://PWM.c \
"

S = "${WORKDIR}"

do_compile() {
	${CC} PWM.c -o test-PWM -lm
}

do_install() {
        install -d ${D}${bindir}
        install -m 0744 ${S}/test-PWM ${D}${bindir}
}
