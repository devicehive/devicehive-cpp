/** @file
@brief The JSON module unit test.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#include <hive/json.hpp>
#include <stdexcept>
#include <iostream>
#include <assert.h>

namespace
{
    using namespace hive;

// assert macro, throws exception
#define MY_ASSERT(cond, msg) \
    std::cout << "checking: " << #cond << "\n"; \
    if (cond) {} else throw std::runtime_error(msg)

json::Value getStringJVal()
{
    return json::Value("test");
}

// test application entry point
/*
Checks for JSON module.
*/
void test_json0()
{
    std::cout << "check for JSON...\n";

    { // check primitive types
        json::Value v0; MY_ASSERT(v0.isNull(), "default value isn't NULL");
        json::Value v1(true); MY_ASSERT(v1.isBool() && v1.asBool(), "bad boolean value");
        json::Value v2(123); MY_ASSERT(v2.isInteger() && v2.asBool() && v2.asInt()==123, "bad integer value");
        json::Value v3(1.23); MY_ASSERT(v3.isDouble() && v3.asBool() && v3.asDouble()==1.23, "bad floating-point value");
        json::Value v4("123"); MY_ASSERT(v4.isString() && v4.asInt()==123 && v4.asString()=="123", "bad string value");

        json::Value v5(v4); MY_ASSERT(v4==v5, "bad copy constructor");
        json::Value v6; v6 = v4; MY_ASSERT(v4==v6, "bad assignment operator");
        json::Value v7("\\/\b\n\fhello \xFF\xFE'\"");

        json::Value v8;
        v8.clear();
        v8.resize(5, v2);

        json::Value v9;
        v9["int"] = v2;
        v9["float"] = v3;
        v9["string"] = v4;
        v9["array"] = v8;
        //v9["self"] = v9;

        std::cout << "\n"
            << v0 << "\n" << v1 << "\n"
            << v2 << "\n" << v3 << "\n"
            << v4 << "\n" << v5 << "\n"
            << v6 << "\n" << v7 << "\n"
            << v8 << "\n" << v9 << "\n";
    }

    { // conversion: asBool()
        MY_ASSERT(json::Value().asBool()==false, "cannot convert NULL to boolean");
        MY_ASSERT(json::Value(0).asBool()==false, "cannot convert 0 to boolean");
        MY_ASSERT(json::Value(123).asBool()==true, "cannot convert 123 to boolean");
        MY_ASSERT(json::Value(0.0).asBool()==false, "cannot convert 0.0 to boolean");
        MY_ASSERT(json::Value(123.123).asBool()==true, "cannot convert 123.123 to boolean");
        MY_ASSERT(json::Value("").asBool()==false, "cannot convert empty string to boolean");
        MY_ASSERT(json::Value("false").asBool()==false, "cannot convert \"false\" string to boolean");
        MY_ASSERT(json::Value("true").asBool()==true, "cannot convert \"true\" string to boolean");

        try { MY_ASSERT(json::Value("test").asBool()==false && false, "can convert from \"test\" string to boolean");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(json::Value::TYPE_ARRAY).asBool()==false && false, "can convert from array to boolean");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(json::Value::TYPE_OBJECT).asBool()==false && false, "can convert from object to boolean");
        } catch (json::error::CastError const&) {}
    }

    { // conversion: asInt()
        MY_ASSERT(json::Value().asInt()==0, "cannot convert NULL to integer");
        MY_ASSERT(json::Value(-123).asInt()==-123, "cannot convert -123 to integer");
        MY_ASSERT(json::Value(-123.0).asInt()==-123, "cannot convert -123.0 to integer");
        MY_ASSERT(json::Value("").asInt()==0, "cannot convert empty string to integer");
        MY_ASSERT(json::Value(" -123 ").asInt()==-123, "cannot convert \"-123\" string to integer");

        try { MY_ASSERT(json::Value("test").asInt()==0 && false, "can convert from \"test\" string to integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value("123456789012345678901234567890").asInt()==0 && false, "can convert from \"123456789012345678901234567890\" string to integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(json::Value::TYPE_ARRAY).asInt()==0 && false, "can convert from array to integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(json::Value::TYPE_OBJECT).asInt()==0 && false, "can convert from object to integer");
        } catch (json::error::CastError const&) {}

        try { MY_ASSERT(json::Value(-1).asUInt64()==0 && false, "can convert from -1 to unsigned integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(-1).asUInt32()==0 && false, "can convert from -1 to unsigned integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(-1).asUInt16()==0 && false, "can convert from -1 to unsigned integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(-1).asUInt8()==0 && false, "can convert from -1 to unsigned integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(UInt64(1000000000)*5).asUInt32()==0 && false, "can convert from 5000000000 to 32-bits unsigned integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(70000).asUInt16()==0 && false, "can convert from 70000 to 16-bits unsigned integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(300).asUInt8()==0 && false, "can convert from 300 to 8-bits unsigned integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(UInt64(1250000000)*2).asInt32()==0 && false, "can convert from 2500000000 to 32-bits signed integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(50000).asInt16()==0 && false, "can convert from 50000 to 16-bits signed integer");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(150).asInt8()==0 && false, "can convert from 150 to 8-bits signed integer");
        } catch (json::error::CastError const&) {}
    }

    { // conversion: asDouble()
        MY_ASSERT(json::Value().asDouble()==0.0, "cannot convert NULL to double");
        MY_ASSERT(json::Value(-123).asDouble()==-123.0, "cannot convert -123 to double");
        MY_ASSERT(json::Value("").asDouble()==0.0, "cannot convert empty string to double");
        MY_ASSERT(json::Value("-123.5").asDouble()==-123.5, "cannot convert \"-123.5\" string to double");

        try { MY_ASSERT(json::Value("test").asDouble()==0.0 && false, "can convert from \"test\" string to double");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(json::Value::TYPE_ARRAY).asDouble()==0.0 && false, "can convert from array to double");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(json::Value::TYPE_OBJECT).asDouble()==0.0 && false, "can convert from object to double");
        } catch (json::error::CastError const&) {}
    }

    { // conversion: asString()
        MY_ASSERT(json::Value().asString()=="", "cannot convert NULL to string");
        MY_ASSERT(json::Value(true).asString()=="true", "cannot convert true to string");
        MY_ASSERT(json::Value(false).asString()=="false", "cannot convert false to string");
        MY_ASSERT(json::Value(-123).asString()=="-123", "cannot convert -123 to string");
        MY_ASSERT(json::Value(-123.123).asString()=="-123.123", "cannot convert -123.123 to string");

        try { MY_ASSERT(json::Value(json::Value::TYPE_ARRAY).asString()=="" && false, "can convert from array to string");
        } catch (json::error::CastError const&) {}
        try { MY_ASSERT(json::Value(json::Value::TYPE_OBJECT).asString()=="" && false, "can convert from object to string");
        } catch (json::error::CastError const&) {}
    }

    { // check parser
        MY_ASSERT(json::fromStr("null") == json::Value(), "cannot parse 'null' token");
        MY_ASSERT(json::fromStr("false") == json::Value(false), "cannot parse 'false' token");
        MY_ASSERT(json::fromStr("true") == json::Value(true), "cannot parse 'true' token");
        MY_ASSERT(json::fromStr("\"hello\"") == json::Value("hello"), "cannot parse string");
        MY_ASSERT(json::fromStr("123") == json::Value(123), "cannot parse integer");
        MY_ASSERT(json::fromStr("123.001") == json::Value(123.001), "cannot parse float-point");

        json::Value v8;
        v8.append(json::Value(1));
        v8.append(json::Value(2));
        v8.append(json::Value(3));
        v8.append(json::Value(4));
        v8.append(json::Value("hello"));
        MY_ASSERT(json::fromStr(" [ 1 , 2 , 3 , 4 , \"hello\" ] ") == v8, "cannot parse float-point");

        json::Value v9;
        v9["array"] = v8;
        v9["int"] = json::Value(123);
        v9["A"]["B"] = json::Value(1);
        v9["A"]["C"] = json::Value(false);
        MY_ASSERT(json::fromStr("#1\n//2\r\n/*3*/{ \"array\" : [ 1 , 2 , 3 , 4 , \"hello\" ], \"int\" : 123, \"A\" : {\"B\":1, \"C\":false} }") == v9, "cannot parse object");

        try { MY_ASSERT(json::fromStr("nul1").asString()=="" && false, "cannot parse \"nul1\"");
        } catch (json::error::SyntaxError const&) {}

        try { MY_ASSERT(json::fromStr("null false").asString()=="" && false, "cannot parse \"null false\"");
        } catch (json::error::SyntaxError const&) {}

        std::cout << "\n\n"
            << json::toStr(v9) << "\n\n"
            << json::toStrH(v9) << "\n";

        json::Value v10 = getStringJVal();
        v10 = json::Value();
        v10 = getStringJVal();
    }

    std::cout << "\n\n";
}

#undef MY_ASSERT

} // local namespace


namespace 
{

bool equalLineByLine(String const& fileName, json::Value const& jval)
{
    StringStream ss;
    json::Formatter::write(ss, jval, true);

    std::ifstream i_file(fileName.c_str());
    if (!i_file.is_open())
        return false;

    while (!i_file.eof() && !ss.eof())
    {
        String L1, L2;

        std::getline(i_file, L1);
        std::getline(ss, L2);

        if (L1 != L2)
            return false;
    }

    if (!i_file.eof())
        return false;
    if (!ss.eof())
        return false;

    return true;
}


/*
Checks for JSON parser (one file).
*/
void test_json1_file(String const& fileName)
{
    std::cout << "checking \"" << fileName << "\": ";
    try
    {
        std::ifstream i_file(fileName.c_str());
        if (i_file && i_file.is_open())
        {
            json::Value jval;
            i_file >> jval;
            //if (!(i_file >> std::ws).eof())
                //throw std::runtime_error("garbage at the end");

            if (equalLineByLine(fileName + ".good", jval))
                std::cout << "OK\n";
            else
            {
                std::cout << "FAILED\n"
                    << json::toStrH(jval)
                    << "\n\n";
            }
        }
        else
            std::cout << "CANNOT OPEN, SKIP\n";
    }
    catch (std::exception const& ex)
    {
        if (equalLineByLine(fileName + ".bad", json::Value(ex.what())))
            std::cout << "OK\n";
        else
        {
            std::cout << "FAILED: "
                << ex.what() << "\n\n";
        }
    }
}


/*
Checks for JSON parser.
*/
void test_json1(String const& testDirName)
{
    std::ifstream i_file((testDirName + "/index").c_str());
    if (i_file.is_open())
    while (!i_file.eof())
    {
        std::string fileName;
        std::getline(i_file, fileName);
        if (!fileName.empty())
        {
            test_json1_file(testDirName
                + "/" + fileName);
        }
    }
}

} // local namespace
