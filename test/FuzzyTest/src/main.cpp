    /*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stddef.h>
#include <iostream>
#include <iterator>
#include <string>
#include <regex>

extern "C" {
#include "gst.h"
}

void gstElementFactoryMake(const gchar* factoryname, const gchar *name)
{
   // 语音记事本使用了两个第三方库，一个是gstreamer，一个是vlc。
   // 使用的vlc库中所有接口均不支持模糊测试。
   // 使用的gstreamer库中所有接口中有三个接口支持模糊测试。
   // 本项目使用模糊测试不支持多接口同时测试，所以采用的是测试一个接口，通过后注释测试代码，接着测试下一个接口。

   // 本接口函数不适用于模糊测试，但是其他接口进行模糊测试时需要依赖此接口。
   gst_init(nullptr, nullptr);

   // 本接口通过一个小时压力测试，测试通过。
//   gst_element_factory_make(factoryname, name);

   // 本接口通过一个小时压力测试，测试通过。
//   auto object = gst_pipeline_new(factoryname);
//   if (nullptr != object) {
//       gst_object_unref(object);
//       object = nullptr;
//   }

   // 本接口通过一个小时压力测试，目前发现一个问题，即入参最后一个字符为符号时(比如')',']'等)，接口会越界报错；其他未发现问题。
   gst_parse_bin_from_description(factoryname, true, nullptr);

   // 本接口函数不适用于模糊测试，但是其他接口进行模糊测试时需要依赖此接口。
   gst_deinit();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size != 0) {
        gchar* tmpchar = new gchar[size + 1];
        memset(tmpchar, 0, size + 1);
        memcpy(tmpchar, data, size);
        tmpchar[size] = '\0';
        std::string testString = tmpchar;
        std::regex strReg("[)）]$");
        std::string newString = std::regex_replace(testString, strReg , "A" );
        gstElementFactoryMake(newString.c_str(), newString.c_str());
//        gstElementFactoryMake(tmpchar, tmpchar);
        delete[] tmpchar;
    }

    return 0;
}

