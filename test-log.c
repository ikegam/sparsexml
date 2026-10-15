#include <stdio.h>
#include <string.h>

#include "test-log.h"

static void log_add(TestLog* log, char kind, const char* text) {
  if (log->count < TEST_LOG_EVENTS) {
    snprintf(log->events[log->count++], TEST_LOG_LENGTH, "%c[%s]", kind, text);
  }
}

static unsigned char log_tag(SXMLExplorer* explorer, char* name) {
  log_add(sxml_get_user_data(explorer), 'T', name);
  return SXMLExplorerContinue;
}

static unsigned char log_content(SXMLExplorer* explorer, char* content) {
  TestLog* log = sxml_get_user_data(explorer);

  if (log->flags & TEST_LOG_CANONICAL) {
    const char* p = content + strspn(content, " \t\r\n");
    char* last = log->count ? log->events[log->count - 1] : NULL;
    if (*p == '\0') {
      return SXMLExplorerContinue;
    }
    if (last != NULL && last[0] == 'C') {
      // Adjacent text, e.g. around a comment that is not recorded:
      // "C[ab]" + "cd" -> "C[abcd]"
      size_t len = strlen(last);
      snprintf(last + len - 1, TEST_LOG_LENGTH - len + 1, "%s]", content);
      return SXMLExplorerContinue;
    }
  }
  log_add(log, 'C', content);
  return SXMLExplorerContinue;
}

static unsigned char log_attribute_key(SXMLExplorer* explorer, char* key) {
  TestLog* log = sxml_get_user_data(explorer);

  log->skip_value = (log->flags & TEST_LOG_CANONICAL) &&
                    (strncmp(key, "xmlns", 5) == 0 || strcmp(key, "xsi:schemaLocation") == 0);
  if (!log->skip_value) {
    const char* colon = strchr(key, ':');
    log_add(log, 'K', (colon && (log->flags & TEST_LOG_LOCAL_KEYS)) ? colon + 1 : key);
  }
  return SXMLExplorerContinue;
}

static unsigned char log_attribute_value(SXMLExplorer* explorer, char* value) {
  TestLog* log = sxml_get_user_data(explorer);

  if (!log->skip_value) {
    log_add(log, 'V', value);
  }
  log->skip_value = 0;
  return SXMLExplorerContinue;
}

static unsigned char log_comment(SXMLExplorer* explorer, char* comment) {
  TestLog* log = sxml_get_user_data(explorer);

  if (log->flags & TEST_LOG_COMMENTS) {
    log_add(log, 'M', comment);
  }
  return SXMLExplorerContinue;
}

void test_log_attach(SXMLExplorer* explorer, TestLog* log, unsigned char flags) {
  memset(log, 0, sizeof(TestLog));
  log->flags = flags;
  sxml_set_user_data(explorer, log);
  sxml_register_func(explorer, log_tag, log_content, log_attribute_key, log_attribute_value);
  sxml_register_comment_func(explorer, log_comment);
}

const char* test_log_string(TestLog* log) {
  size_t len = 0;
  unsigned int i;

  log->joined[0] = '\0';
  for (i = 0; i < log->count && len < sizeof(log->joined) - 1; i++) {
    snprintf(log->joined + len, sizeof(log->joined) - len, "%s", log->events[i]);
    len += strlen(log->joined + len);
  }
  return log->joined;
}

int test_log_find(const TestLog* log, const char* event) {
  unsigned int i;
  for (i = 0; i < log->count; i++) {
    if (strcmp(log->events[i], event) == 0) {
      return (int)i;
    }
  }
  return -1;
}
