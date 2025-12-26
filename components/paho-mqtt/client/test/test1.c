/*******************************************************************************
 * Copyright (c) 2009, 2017 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial implementation for embedded C client
 *******************************************************************************/


/**
 * @file
 * Tests for the Paho embedded C "high" level client
 */


#include "MQTTClient.h"
#include <string.h>
#include <stdlib.h>

#if !defined(_WINDOWS)
  #include <sys/time.h>
  #include <sys/socket.h>
  #include <unistd.h>
  #include <errno.h>
#else
  #include <windows.h>
  #define setenv(a, b, c) _putenv_s(a, b)
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void usage(void)
{
	printf("help!!\n");
	exit(EXIT_FAILURE);
}

struct Options
{
	char* host;         /**< connection to system under test. */
  int port;
	char* proxy_host;
  int proxy_port;
	int verbose;
	int test_no;
	int MQTTVersion;
	int iterations;
} options =
{
	"localhost",
  1883,
	"localhost",
	1885,
	0, //verbose
	0, //test_no
	4,
	1,
};

void getopts(int argc, char** argv)
{
	int count = 1;

	while (count < argc)
	{
		if (strcmp(argv[count], "--test_no") == 0)
		{
			if (++count < argc)
				options.test_no = atoi(argv[count]);
			else
				usage();
		}
		else if (strcmp(argv[count], "--host") == 0)
		{
			if (++count < argc)
			{
				options.host = argv[count];
				printf("\nSetting host to %s\n", options.host);
			}
			else
				usage();
		}
    else if (strcmp(argv[count], "--port") == 0)
    {
      if (++count < argc)
      {
        options.port = atoi(argv[count]);
        printf("\nSetting port to %d\n", options.port);
      }
      else
        usage();
    }
		else if (strcmp(argv[count], "--proxy_host") == 0)
		{
			if (++count < argc)
      {
				options.proxy_host = argv[count];
        printf("\nSetting proxy_host to %s\n", options.proxy_host);
      }
			else
				usage();
		}
    else if (strcmp(argv[count], "--proxy_port") == 0)
    {
      if (++count < argc)
      {
        options.proxy_port = atoi(argv[count]);
        printf("\nSetting proxy_port to %d\n", options.proxy_port);
      }
      else
        usage();
    }
		else if (strcmp(argv[count], "--MQTTversion") == 0)
		{
			if (++count < argc)
			{
				options.MQTTVersion = atoi(argv[count]);
				printf("setting MQTT version to %d\n", options.MQTTVersion);
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--iterations") == 0)
		{
			if (++count < argc)
				options.iterations = atoi(argv[count]);
			else
				usage();
		}
		else if (strcmp(argv[count], "--verbose") == 0)
		{
			options.verbose = 1;
			printf("\nSetting verbose on\n");
		}
		count++;
	}
}


#define LOGA_DEBUG 0
#define LOGA_INFO 1
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>
void MyLog(int LOGA_level, char* format, ...)
{
	static char msg_buf[256];
	va_list args;
	struct timeb ts;

	struct tm *timeinfo;

	if (LOGA_level == LOGA_DEBUG && options.verbose == 0)
	  return;

	ftime(&ts);
	timeinfo = localtime(&ts.time);
	strftime(msg_buf, 80, "%Y%m%d %H%M%S", timeinfo);

	sprintf(&msg_buf[strlen(msg_buf)], ".%.3hu ", ts.millitm);

	va_start(args, format);
	vsnprintf(&msg_buf[strlen(msg_buf)], sizeof(msg_buf) - strlen(msg_buf), format, args);
	va_end(args);

	printf("%s\n", msg_buf);
	fflush(stdout);
}


#if defined(WIN32) || defined(_WINDOWS)
#define mqsleep(A) Sleep(1000*A)
#define START_TIME_TYPE DWORD
static DWORD start_time = 0;
START_TIME_TYPE start_clock(void)
{
	return GetTickCount();
}
#elif defined(AIX)
#define mqsleep sleep
#define START_TIME_TYPE struct timespec
START_TIME_TYPE start_clock(void)
{
	static struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);
	return start;
}
#else
#define mqsleep sleep
#define START_TIME_TYPE struct timeval
/* TODO - unused - remove? static struct timeval start_time; */
START_TIME_TYPE start_clock(void)
{
	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	return start_time;
}
#endif


#if defined(WIN32)
long elapsed(START_TIME_TYPE start_time)
{
	return GetTickCount() - start_time;
}
#elif defined(AIX)
#define assert(a)
long elapsed(struct timespec start)
{
	struct timespec now, res;

	clock_gettime(CLOCK_REALTIME, &now);
	ntimersub(now, start, res);
	return (res.tv_sec)*1000L + (res.tv_nsec)/1000000L;
}
#else
long elapsed(START_TIME_TYPE start_time)
{
	struct timeval now, res;

	gettimeofday(&now, NULL);
	timersub(&now, &start_time, &res);
	return (res.tv_sec)*1000 + (res.tv_usec)/1000;
}
#endif


#define assert(a, b, c, d) myassert(__FILE__, __LINE__, a, b, c, d)
#define assert1(a, b, c, d, e) myassert(__FILE__, __LINE__, a, b, c, d, e)

int tests = 0;
int failures = 0;
FILE* xml;
START_TIME_TYPE global_start_time;
char output[3000];
char* cur_output = output;


void write_test_result(void)
{
	long duration = elapsed(global_start_time);

	fprintf(xml, " time=\"%ld.%.3ld\" >\n", duration / 1000, duration % 1000);
	if (cur_output != output)
	{
		fprintf(xml, "%s", output);
		cur_output = output;
	}
	fprintf(xml, "</testcase>\n");
}


void myassert(char* filename, int lineno, char* description, int value, char* format, ...)
{
	++tests;
	if (!value)
	{
		va_list args;

		++failures;
		MyLog(LOGA_INFO, "Assertion failed, file %s, line %d, description: %s\n", filename, lineno, description);

		va_start(args, format);
		vprintf(format, args);
		va_end(args);

		cur_output += sprintf(cur_output, "<failure type=\"%s\">file %s, line %d </failure>\n",
                        description, filename, lineno);
	}
	else
		MyLog(LOGA_DEBUG, "Assertion succeeded, file %s, line %d, description: %s", filename, lineno, description);
}


static volatile MessageData* test1_message_data = NULL;
static MQTTMessage pubmsg;

void echo_msg_arrived_cb(MessageData* md)
{
  test1_message_data = md;
  MQTTMessage* m = md->message;

	assert("Good message lengths", pubmsg.payloadlen == m->payloadlen,
         "payloadlen was %d", m->payloadlen);

  if (pubmsg.payloadlen == m->payloadlen)
      assert("Good message contents", memcmp(m->payload, pubmsg.payload, m->payloadlen) == 0,
          "payload was %s", m->payload);
}


/*********************************************************************

Test1: single-threaded client

*********************************************************************/
void test1_sendAndReceive(MQTTClient* c, int qos, char* test_topic)
{
	char* topicName = NULL;
	int topicLen;
	int i = 0;
	int iterations = 50;
	int rc;
  int wait_seconds;

	MyLog(LOGA_DEBUG, "%d messages at QoS %d", iterations, qos);
  memset(&pubmsg, '\0', sizeof(pubmsg));
	pubmsg.payload = "a much longer message that we can shorten to the extent that we need to payload up to 11";
	pubmsg.payloadlen = 11;
	pubmsg.qos = qos;
	pubmsg.retained = 0;
  pubmsg.dup = 0;

	for (i = 0; i < iterations; ++i)
	{
    test1_message_data = NULL;
		rc = mqtt_publish(c, test_topic, &pubmsg);
		assert("Good rc from publish", rc == SUCCESS, "rc was %d", rc);

    /* wait for the message to be received */
    wait_seconds = 10;
		while ((test1_message_data == NULL) && (wait_seconds-- > 0))
		{
      mqtt_yield(c, 100);
		}
		assert("Message Arrived", wait_seconds > 0, "Time out waiting for message %d\n", i);

		if (!test1_message_data)
			printf("No message received within timeout period\n");
	}

	/* wait to receive any outstanding messages */
  wait_seconds = 2;
  while (wait_seconds-- > 0)
  {
      mqtt_yield(c, 1000);
  }
}


int test1(struct Options options)
{
	int subsqos = 2;
	Network n;
	MQTTClient c;
	int rc = 0;
	char* test_topic = "C client test1";
	MQTTPacket_willOptions wopts;
	unsigned char buf[100];
	unsigned char readbuf[100];

  	printf("test1\n");
	fprintf(xml, "<testcase classname=\"test1\" name=\"single threaded client using receive\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 1 - single threaded client using receive");

	NetworkInit(&n);
	NetworkConnect(&n, options.host, options.port);
	mqtt_client_init(&c, &n, 1000, buf, 100, readbuf, 100);

	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	data.willFlag = 1;
	data.MQTTVersion = options.MQTTVersion;
	data.clientID.cstring = "single-threaded-test";
	data.username.cstring = "testuser";
	data.password.cstring = "testpassword";

	data.keepAliveInterval = 20;
	data.cleansession = 1;

	data.will.message.cstring = "will message";
	data.will.qos = 1;
	data.will.retained = 0;
	data.will.topicName.cstring = "will topic";

	MyLog(LOGA_DEBUG, "Connecting");
  	rc = mqtt_connect(&c, &data);
	assert("Good rc from connect", rc == SUCCESS, "rc was %d", rc);
	if (rc != SUCCESS)
		goto exit;

	rc = mqtt_subscribe(&c, test_topic, subsqos, echo_msg_arrived_cb);
	assert("Good rc from subscribe", rc == SUCCESS, "rc was %d", rc);

	test1_sendAndReceive(&c, 0, test_topic);
	test1_sendAndReceive(&c, 1, test_topic);
	test1_sendAndReceive(&c, 2, test_topic);

	MyLog(LOGA_DEBUG, "Stopping\n");

	rc = mqtt_unsubscribe(&c, test_topic);
	assert("Unsubscribe successful", rc == SUCCESS, "rc was %d", rc);
	rc = mqtt_disconnect(&c);
	assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);

	/* Just to make sure we can connect again */
  NetworkConnect(&n, options.host, options.port);
  rc = mqtt_connect(&c, &data);
	assert("Connect successful",  rc == SUCCESS, "rc was %d", rc);
	rc = mqtt_disconnect(&c);
	assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);

exit:
	MyLog(LOGA_INFO, "TEST1: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}


/*********************************************************************

Test 2: connack return data

sessionPresent

*********************************************************************/
int test2(struct Options options)
{
	int subsqos = 2;
  Network n;
	MQTTClient c;
	int rc = 0;
	char* test_topic = "C client test2";
  MQTTPacket_willOptions wopts;
  unsigned char buf[100];
  unsigned char readbuf[100];
  MQTTConnackData connack;
  MQTTSubackData suback;

	fprintf(xml, "<testcase classname=\"test2\" name=\"connack return data\"");
	global_start_time = start_clock();
	failures = 0;
	MyLog(LOGA_INFO, "Starting test 2 - connack return data");

  NetworkInit(&n);
  NetworkConnect(&n, options.host, options.port);
  mqtt_client_init(&c, &n, 1000, buf, 100, readbuf, 100);

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.willFlag = 1;
  data.MQTTVersion = options.MQTTVersion;
  data.clientID.cstring = "connack-return-data";
  data.username.cstring = "testuser";
  data.password.cstring = "testpassword";

  data.keepAliveInterval = 20;
  data.cleansession = 1;

	data.will.message.cstring = "will message";
	data.will.qos = 1;
	data.will.retained = 0;
	data.will.topicName.cstring = "will topic";

	MyLog(LOGA_DEBUG, "Connecting");
  rc = mqtt_connect(&c, &data);
	assert("Good rc from connect", rc == SUCCESS, "rc was %d", rc);
	if (rc != SUCCESS)
		goto exit;

	rc = mqtt_disconnect(&c);
	assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);
  NetworkDisconnect(&n);

  /* now connect cleansession false */
  NetworkConnect(&n, options.host, options.port);
  data.cleansession = 0;
  rc = mqtt_connect_with_results(&c, &data, &connack);
  assert("Good rc from connect", rc == SUCCESS, "rc was %d", rc);

  assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
  assert("Session present is 0", connack.sessionPresent == 0,
         "sessionPresent was %d", connack.sessionPresent);

  /* set up some state */
  rc = mqtt_subscribe_with_results(&c, test_topic, subsqos, echo_msg_arrived_cb, &suback);
  assert("Good rc from subscribe", rc == SUCCESS, "rc was %d", rc);
  assert("Good granted QoS", suback.grantedQoS == subsqos,
         "granted QoS was %d", suback.grantedQoS);

  rc = mqtt_disconnect(&c);
  assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);
  NetworkDisconnect(&n);

	/* Connect and check sessionPresent */
  NetworkConnect(&n, options.host, options.port);
  rc = mqtt_connect_with_results(&c, &data, &connack);
	assert("Connect successful",  rc == SUCCESS, "rc was %d", rc);

  assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
  assert("Session present is 1", connack.sessionPresent == 1,
           "sessionPresent was %d", connack.sessionPresent);

	rc = mqtt_disconnect(&c);
	assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);
  NetworkDisconnect(&n);

  /* Connect and check sessionPresent is cleared */
  data.cleansession = 1;
  NetworkConnect(&n, options.host, options.port);
  rc = mqtt_connect_with_results(&c, &data, &connack);
  assert("Connect successful",  rc == SUCCESS, "rc was %d", rc);

  assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
  assert("Session present is 0", connack.sessionPresent == 0,
           "sessionPresent was %d", connack.sessionPresent);

  rc = mqtt_disconnect(&c);
  assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);
  NetworkDisconnect(&n);

exit:
	MyLog(LOGA_INFO, "TEST1: test %s. %d tests run, %d failures.",
			(failures == 0) ? "passed" : "failed", tests, failures);
	write_test_result();
	return failures;
}

/*********************************************************************

Test 3: client session state

*********************************************************************/
static volatile MessageData* test2_message_data = NULL;

void messageArrived2(MessageData* md)
{
    test2_message_data = md;
	  MQTTMessage *m = md->message;

    assert("Good message lengths", pubmsg.payloadlen == m->payloadlen,
         "payloadlen was %d", m->payloadlen);

    if (pubmsg.payloadlen == m->payloadlen)
        assert("Good message contents", memcmp(m->payload, pubmsg.payload, m->payloadlen) == 0,
          "payload was %s", m->payload);
}


int check_subs_exist(MQTTClient* c, const char* test_topic, int which)
{
    int rc = FAILURE;
    int wait_seconds = 0;

    memset(&pubmsg, '\0', sizeof(pubmsg));
    pubmsg.payload = (void*)"a much longer message that we can shorten to the extent that we need to payload up to 11";
    pubmsg.payloadlen = 11;
    pubmsg.qos = QOS2;
    pubmsg.retained = 0;
    pubmsg.dup = 0;

    test1_message_data = test2_message_data = NULL;
    rc = mqtt_publish(c, test_topic, &pubmsg);
    assert("Good rc from publish", rc == SUCCESS, "rc was %d", rc);

    /* wait for the message to be received */
    wait_seconds = 10;
    while (wait_seconds-- > 0)
    {
        mqtt_yield(c, 100);
    }

    rc = (((which == 1 || which == 3) && test1_message_data) ||
         (which == 2 && test1_message_data == NULL)) ? SUCCESS : FAILURE;
    assert("test1 subscription", rc == SUCCESS, "test1_message_data %p\n",
            test1_message_data);
    rc = (((which == 2 || which == 3) && test2_message_data) ||
         (which == 1 && test2_message_data == NULL)) ? SUCCESS : FAILURE;
    assert("test2 subscription", rc == SUCCESS, "test2_message_data %p\n",
             test2_message_data);
    return rc;
}


int test3(struct Options options)
{
  enum QoS subsqos = QOS2;
  Network n;
  MQTTClient c;
	int rc;
  const char* test_topic = "C client test3";
  int wait_seconds = 0;
  unsigned char buf[100];
  unsigned char readbuf[100];
  MQTTConnackData connack;
  MQTTSubackData suback;

  fprintf(xml, "<testcase classname=\"test3\" name=\"session state\"");
  global_start_time = start_clock();
  failures = 0;
  MyLog(LOGA_INFO, "Starting test 3 - session state");

  NetworkInit(&n);
  mqtt_client_init(&c, &n, 1000, buf, 100, readbuf, 100);

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.willFlag = 1;
  data.MQTTVersion = options.MQTTVersion;
  data.clientID.cstring = (char*)"connack-session-state";
  data.username.cstring = (char*)"testuser";
  data.password.cstring = (char*)"testpassword";

  data.keepAliveInterval = 10;
  data.cleansession = 1;

  data.will.message.cstring = (char*)"will message";
  data.will.qos = 1;
  data.will.retained = 0;
  data.will.topicName.cstring = (char*)"will topic";

  assert("Not connected", mqtt_is_connected(&c) == 0,
         "is_connected was %d", mqtt_is_connected(&c));

  MyLog(LOGA_DEBUG, "Connecting");
  rc = NetworkConnect(&n, options.host, options.port);
  assert("Good rc from TCP connect", rc == SUCCESS, "rc was %d", rc);
  if (rc != SUCCESS)
    goto exit;

  rc = mqtt_connect_with_results(&c, &data, &connack);
  assert("Good rc from connect", rc == SUCCESS, "rc was %d", rc);
  if (rc != SUCCESS)
    goto exit;

  assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
  assert("Session present is 0", connack.sessionPresent == 0,
         "sessionPresent was %d", connack.sessionPresent);

  assert("Good rc in connack", mqtt_is_connected(&c) == 1,
                "is_connected was %d", mqtt_is_connected(&c));

  rc = mqtt_disconnect(&c);
  assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);
  NetworkDisconnect(&n);

  /* reconnect with cleansession false */
  data.cleansession = 0;
  rc = NetworkConnect(&n, options.proxy_host, options.proxy_port);
  assert("TCP connect successful",  rc == SUCCESS, "rc was %d", rc);
  rc = mqtt_connect_with_results(&c, &data, &connack);
  assert("Connect successful",  rc == SUCCESS, "rc was %d", rc);

  assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
  assert("Session present is 0", connack.sessionPresent == 0,
           "sessionPresent was %d", connack.sessionPresent);

  rc = mqtt_subscribe_with_results(&c, test_topic, subsqos, echo_msg_arrived_cb, &suback);
  assert("Good rc from subscribe", rc == SUCCESS, "rc was %d", rc);
  assert("Granted QoS rc from subscribe", suback.grantedQoS == QOS2,
         "rc was %d", suback.grantedQoS);

  check_subs_exist(&c, test_topic, 1);

  rc = mqtt_subscribe_with_results(&c, test_topic, subsqos, messageArrived2, &suback);
  assert("Good rc from subscribe", rc == SUCCESS, "rc was %d", rc);
  assert("Granted QoS rc from subscribe", suback.grantedQoS == QOS2,
                  "rc was %d", suback.grantedQoS);

  check_subs_exist(&c, test_topic, 2);

  rc = mqtt_disconnect(&c);
  assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);
  NetworkDisconnect(&n);

  /* reconnect with cleansession false */
  data.cleansession = 0;
  rc = NetworkConnect(&n, options.proxy_host, options.proxy_port);
  assert("TCP connect successful",  rc == SUCCESS, "rc was %d", rc);
  rc = mqtt_connect_with_results(&c, &data, &connack);
  assert("Connect successful",  rc == SUCCESS, "rc was %d", rc);

  assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
  assert("Session present is 1", connack.sessionPresent == 1,
           "sessionPresent was %d", connack.sessionPresent);

  check_subs_exist(&c, test_topic, 2);

  rc = mqtt_subscribe_with_results(&c, test_topic, subsqos, echo_msg_arrived_cb, &suback);
  assert("Good rc from subscribe", rc == SUCCESS, "rc was %d", rc);
  assert("Granted QoS rc from subscribe", suback.grantedQoS == QOS2,
            "rc was %d", suback.grantedQoS);

  check_subs_exist(&c, test_topic, 1);

  // cause a connection FAILURE
  memset(&pubmsg, '\0', sizeof(pubmsg));
  pubmsg.payload = (void*)"TERMINATE";
  pubmsg.payloadlen = strlen((char*)pubmsg.payload);
  pubmsg.qos = QOS0;
  pubmsg.retained = 0;
  pubmsg.dup = 0;
  rc = mqtt_publish(&c, "MQTTSAS topic", &pubmsg);
  assert("Good rc from publish", rc == SUCCESS, "rc was %d", rc);

  // wait for failure to be noticed by keepalive
  wait_seconds = 20;
  while (mqtt_is_connected(&c) && (wait_seconds-- > 0))
  {
      mqtt_yield(&c, 1000);
  }
  assert("Disconnected", !mqtt_is_connected(&c), "isConnected was %d",
         mqtt_is_connected(&c));
  NetworkDisconnect(&n);

  /* reconnect with cleansession false */
  data.cleansession = 0;
  rc = NetworkConnect(&n, options.host, options.port);
  assert("TCP connect successful",  rc == SUCCESS, "rc was %d", rc);
  rc = mqtt_connect_with_results(&c, &data, &connack);
  assert("Connect successful",  rc == SUCCESS, "rc was %d", rc);

  assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
  assert("Session present is 1", connack.sessionPresent == 1,
           "sessionPresent was %d", connack.sessionPresent);

  check_subs_exist(&c, test_topic, 1);

  rc = mqtt_subscribe_with_results(&c, test_topic, subsqos, messageArrived2, &suback);
  assert("Good rc from subscribe", rc == SUCCESS, "rc was %d", rc);
  assert("Granted QoS rc from subscribe", suback.grantedQoS == QOS2,
                  "rc was %d", suback.grantedQoS);

  check_subs_exist(&c, test_topic, 2);

  rc = mqtt_disconnect(&c);
  assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);
  NetworkDisconnect(&n);

  /* reconnect with cleansession true to clean up both server and client state */
  data.cleansession = 1;
  rc = NetworkConnect(&n, options.host, options.port);
  assert("TCP connect successful",  rc == SUCCESS, "rc was %d", rc);
  rc = mqtt_connect_with_results(&c, &data, &connack);
  assert("Connect successful",  rc == SUCCESS, "rc was %d", rc);

  assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
  assert("Session present is 0", connack.sessionPresent == 0,
           "sessionPresent was %d", connack.sessionPresent);

  rc = mqtt_subscribe_with_results(&c, test_topic, subsqos, messageArrived2, &suback);
  assert("Good rc from subscribe", rc == SUCCESS, "rc was %d", rc);
  assert("Granted QoS rc from subscribe", suback.grantedQoS == QOS2,
                  "rc was %d", suback.grantedQoS);

  check_subs_exist(&c, test_topic, 2);

  rc = mqtt_disconnect(&c);
  assert("Disconnect successful", rc == SUCCESS, "rc was %d", rc);
  NetworkDisconnect(&n);

exit:
  MyLog(LOGA_INFO, "TEST2: test %s. %d tests run, %d failures.",
      (failures == 0) ? "passed" : "failed", tests, failures);
  write_test_result();
  return failures;
}

int main(int argc, char** argv)
{
	int rc = 0;
 	int (*tests[])() = {NULL, test1, test2, test3};
	int i;

	xml = fopen("TEST-test1.xml", "w");
	fprintf(xml, "<testsuite name=\"test1\" tests=\"%d\">\n", (int)(ARRAY_SIZE(tests) - 1));

	getopts(argc, argv);

	for (i = 0; i < options.iterations; ++i)
	{
	 	if (options.test_no == 0)
		{ /* run all the tests */
 		   	for (options.test_no = 1; options.test_no < ARRAY_SIZE(tests); ++options.test_no)
				rc += tests[options.test_no](options); /* return number of failures.  0 = test succeeded */
		}
		else
 		   	rc = tests[options.test_no](options); /* run just the selected test */
	}

 	if (rc == 0)
		MyLog(LOGA_INFO, "verdict pass");
	else
		MyLog(LOGA_INFO, "verdict fail");

	fprintf(xml, "</testsuite>\n");
	fclose(xml);
	return rc;
}
