CC = gcc
CFLAGS = -Wall -Wextra -I../firmware/shared -lm

all: test_protocol test_crypto test_state_machine test_sms_parser test_geofence

test_protocol: test_protocol.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

test_crypto: test_crypto.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

test_state_machine: test_state_machine.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

test_sms_parser: test_sms_parser.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

test_geofence: test_geofence.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

clean:
	rm -f test_protocol test_crypto test_state_machine test_sms_parser test_geofence

.PHONY: all clean
