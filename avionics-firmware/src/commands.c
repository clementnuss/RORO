#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <shell.h>
#include <string.h>

#include "gnss.h"
#include "servo.h"
#include "main.h"
#include "msgbus/type_print.h"

static void cmd_bootloader(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)chp;
    (void)argc;
    (void)argv;
    reboot_st_bootloader();
}

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
    static const char *states[] = {CH_STATE_NAMES};
    thread_t *tp;

    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: threads\r\n");
        return;
    }
    chprintf(chp, "    addr    stack prio refs     state       time\r\n");
    tp = chRegFirstThread();
    do {
        chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %10lu %s\r\n",
                (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
                (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
                states[tp->p_state], (uint32_t)tp->p_time, tp->p_name);
        tp = chRegNextThread(tp);
    } while (tp != NULL);
}

static void cmd_gnss_pwr(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    if (argc < 1) {
        chprintf(chp, "usage: gnss_pwr on|off\n");
    }

    if (!strcmp(argv[0], "on")) {
        chprintf(chp, "GNSS on.\n");
        palClearPad(GPIOC, GPIOC_GPS_EN_N);
    } else if (!strcmp(argv[0], "off")) {
        chprintf(chp, "GNSS off.\n");
        palSetPad(GPIOC, GPIOC_GPS_EN_N);
    }
}

static void cmd_gnss_config(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    chprintf(chp, "Configure GNSS UART pin as input... ");
    palSetPadMode(GPIOC, GPIOC_UART6_TX, PAL_MODE_INPUT);
    chprintf(chp, "Done\n");
    chprintf(chp, "You can now configure the GNSS.\n");
}

static void cmd_gnss_switch(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)chp;
    (void)argc;
    (void)argv;
    if (argc < 1) {
        chprintf(chp, "usage: gnss_switch up|down\n");
    }

    if (!strcmp(argv[0], "up")) {
        chprintf(chp, "select upper GNSS antenna.\n");
        gnss_switch_upper_antenna();
    } else if (!strcmp(argv[0], "down")) {
        chprintf(chp, "select lower GNSS antenna.\n");
        gnss_switch_lower_antenna();
    }
}

static void cmd_gnss_forward(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)chp;
    (void)argc;
    (void)argv;
    while(1) {
        static uint8_t buf[100];
        size_t len = sdReadTimeout(&GNSS_SERIAL, buf, sizeof(buf), MS2ST(10));
        if (len > 0) {
            chSequentialStreamWrite(chp, buf, len);
        }
        if (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == 'q') {
            return;
        }
    }
}

/* shared commands */
void cmd_nosecone(BaseSequentialStream *chp, int argc, char *argv[])
{
    if (argc < 1) {
        chprintf(chp, "usage: nosecone lock|unlock\n");
    }

    if (!strcmp(argv[0], "lock")) {
        chprintf(chp, "Locking nosecone...\n");
        nosecone_locked = true;
    } else if (!strcmp(argv[0], "unlock")) {
        chprintf(chp, "Unlocking nosecone...\n");
        nosecone_locked = false;
    }
}

static void cmd_topic_print(BaseSequentialStream *stream, int argc, char *argv[]) {
    if (argc != 1) {
        chprintf(stream, "usage: topic_print name\n");
        return;
    }
    msgbus_subscriber_t sub;
    if (msgbus_topic_subscribe(&sub, &bus, argv[0], MSGBUS_TIMEOUT_IMMEDIATE)) {
        if (msgbus_subscriber_topic_is_valid(&sub)) {
            msgbus_topic_t *topic = msgbus_subscriber_get_topic(&sub);
            const msgbus_type_definition_t *type = msgbus_topic_get_type(topic);

            char buf[type->struct_size];
            // void *buf = malloc(type->struct_size);
            // if (buf == NULL) {
            //     chprintf(stream, "malloc failed\n");
            //     return;
            // }
            msgbus_subscriber_read(&sub, buf);
            msgbus_print_type((void (*)(void *, const char *, ...))chprintf,
                              stream, type, buf);
            // free(buf);
        } else {
            chprintf(stream, "topic not published yet\n");
            return;
        }
    } else {
        chprintf(stream, "topic doesn't exist\n");
        return;
    }
}

static void cmd_topic_list(BaseSequentialStream *stream, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    msgbus_topic_t *topic = msgbus_iterate_topics(&bus);
    while (topic != NULL) {
        chprintf(stream, "%s\n", msgbus_topic_get_name(topic));
        topic = msgbus_iterate_topics_next(topic);
    }
}

const ShellCommand shell_commands[] = {
    {"threads", cmd_threads},
    {"bootloader", cmd_bootloader},
    {"nosecone", cmd_nosecone},
    {"gnss_pwr", cmd_gnss_pwr},
    {"gnss_config", cmd_gnss_config},
    {"gnss_switch", cmd_gnss_switch},
    {"gnss_forward", cmd_gnss_forward},
    {"topic_list", cmd_topic_list},
    {"topic_print", cmd_topic_print},
    {NULL, NULL}
};