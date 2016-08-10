// Copyright 2016 The Bazel Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "src/tools/singlejar/test_util.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>

#include <string>

#include "src/main/cpp/blaze_util.h"
#include "src/main/cpp/util/file.h"
#include "src/main/cpp/util/strings.h"

#include "gtest/gtest.h"

namespace singlejar_test_util {

bool AllocateFile(const std::string &name, size_t size) {
  int fd = open(name.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0777);
  if (fd < 0) {
    perror(name.c_str());
    return false;
  }
  if (size) {
    if (ftruncate(fd, size) == 0) {
      return close(fd) == 0;
    } else {
      auto last_error = errno;
      close(fd);
      errno = last_error;
      return false;
    }
  } else {
    return close(fd) == 0;
  }
}

int RunCommand(const char *cmd, ...) {
  std::string args_string(cmd);
  va_list ap;
  va_start(ap, cmd);
  for (const char *arg = va_arg(ap, const char *); arg;
       arg = va_arg(ap, const char *)) {
    args_string += ' ';
    args_string += arg;
  }
  va_end(ap);
  fprintf(stderr, "Arguments: %s\n", args_string.c_str());
  return system(args_string.c_str());
}

// List zip file contents.
void LsZip(const char *zip_name) {
#if !defined(__APPLE__)
  std::string command = (std::string("unzip -v ") + zip_name).c_str();
  system(command.c_str());
#endif
}

std::string OutputFilePath(const std::string &relpath) {
  const char *out_dir = getenv("TEST_TMPDIR");
  return blaze_util::JoinPath((nullptr == out_dir) ? "." : out_dir,
                              relpath.c_str());
}

int VerifyZip(const std::string &zip_path) {
  std::string verify_command;
  blaze_util::StringPrintf(&verify_command, "zip -Tv %s", zip_path.c_str());
  return system(verify_command.c_str());
}

std::string GetEntryContents(const std::string &zip_path,
                             const std::string &entry_name) {
  std::string contents;
  std::string command;
  blaze_util::StringPrintf(&command, "unzip -p %s %s", zip_path.c_str(),
                           entry_name.c_str());
  FILE *fp = popen(command.c_str(), "r");
  if (!fp) {
    ADD_FAILURE() << "Command " << command << " failed.";
    return std::string("");
  }

  char buf[1024];
  while (fgets(buf, sizeof(buf), fp)) {
    contents.append(buf);
  }
  if (feof(fp) && !ferror(fp) && !pclose(fp)) {
    return contents;
  }
  ADD_FAILURE() << "Command " << command << " failed on close.";
  return std::string("");
}

}  // namespace singlejar_test_util