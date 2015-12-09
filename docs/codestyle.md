Code style guidelines {#page_hive_style}
==========================================

Code style guidelines are based on [WebKit](http://www.webkit.org/coding/coding-style.html) guidelines with a few differences.

Indentation
-----------

Use spaces, not tabs.

The indent size is 4 spaces.

- Right:
~~~{.cpp}
int main()
{
    return 0;
}
~~~

- Wrong:
~~~{.cpp}
int main()
{
        return 0;
}
~~~

The contents of an outermost `namespace` block (and any nested namespaces with the same scope)
should not be indented. The nested namespace definition should be indented.
All implementaion should be placed in separate `namespace` block to use advantages
of text editor with folding support.

- Right:
~~~{.cpp}
// core.h
namespace core
{

class Foo
{
    Foo();
    ...
};

class Bar
{
    Bar();
    ...
};

    namespace nested
    {

class Fail
{
    Fail();
    ...
};

    } // nested namespace
} // core namespace
~~~
~~~{.cpp}
// core.cpp
// Foo
namespace core
{

Foo::Foo()
{
    ...
}

} // Foo

// Bar
namespace core
{

Bar::Bar()
{
    ...
}

} // Bar
~~~

- Wrong:
~~~{.cpp}
// core.h
namespace core {

    class Foo {
        Foo();
        ...
    };

    class Bar {
        Bar();
        ...
    };

    namespace nested {

        class Fail {
            Fail();
            ...
        };

    }
}
~~~
~~~{.cpp}
// core.cpp
namespace core {

    Foo::Foo() {
        ...
    }

    Bar::Bar() {
        ...
    }

}
~~~

A `case` label should be indented with its `switch` statement. The `case` statement is indented with its `case` label.

- Right:
~~~{.cpp}
switch (condition)
{
    case FOO:
        ++i;
        break;

    case BAR:
    {
        --i;
    } break;

    default:
        i = 0;
}
~~~

- Wrong:
~~~{.cpp}
switch (condition) {
case FOO:
++i; break;

case BAR:
--i;
break;

default: i = 0;
}
~~~


Boolean expressions at the same nesting level that span multiple lines
should have their operators on the left side of the line instead of the right side.

- Right:
~~~{.cpp}
if (name == "Foo"
    || name == "Bar"
    || (name == "Fail" && val != "?"))
        return;
~~~

- Wrong:
~~~{.cpp}
if (name == "Foo" ||
    name == "Bar" ||
    (name == "Fail" && val != "?"))
    return;
~~~


Spacing
-------

Do not place spaces around unary operators.

- Right:
~~~{.cpp}
i++;
~~~

- Wrong:
~~~{.cpp}
i ++;
~~~

Do place spaces around binary and ternary operators.
It is possible not to put spaces to emphasise priority.

- Right:
~~~{.cpp}
y = m*x + b;
f(a, b);
c = a | b;
return condition ? 1 : 0;
~~~

- Wrong:
~~~{.cpp}
y=m*x+b;
f(a,b);
c = a|b;
return condition?1:0;
~~~

Do not place spaces before comma and semicolon.

- Right:
~~~{.cpp}
for (int i = 0; i < 5; ++i)
    doSmth();

f(a, b);
~~~

- Wrong:
~~~{.cpp}
for (int i = 0 ; i < 5 ; ++i)
    doSmth();

f(a , b) ;
~~~

Place spaces between control statements and their parentheses.

- Right:
~~~{.cpp}
if (condition)
    doIt();
~~~

- Wrong:
~~~{.cpp}
if(condition)
    doIt();
~~~

Do not place spaces between a function and its parentheses, or between a parenthesis and its content.

- Right:
~~~{.cpp}
f(a, b);
~~~

- Wrong:
~~~{.cpp}
f (a, b);
f( a, b );
~~~

Empty lines shouldn't contain any spaces.


Line breaking
-------------

Each statement should get its own line.

- Right:
~~~{.cpp}
x++;
y++;
if (condition)
    doIt();
~~~

- Wrong:
~~~{.cpp}
x++; y++;
if (condition) doIt();
~~~


An `else` statement should should line up with the `if` statement.

- Right:
~~~{.cpp}
if (condition)
{
    ...
}
else
{
    ...
}

if (condition)
    doSmth();
else
    doSmthOther();

if (condition)
    doSmth();
else
{
    ...
}
~~~

- Wrong:
~~~{.cpp}
if (condition) {
    ...
}
else {
    ...
}

if (condition) doSmth(); else doSmthOther();

if (condition) doSmth(); else {
    ...
}
~~~


Braces
------

Place each brace on its own line.

- Right:
~~~{.cpp}
class MyClass
{
    ...
};

namespace core
{
    ...
}

int main()
{
    for (int i = 0; i < 10; ++i)
    {
        ...
    }

    ...
}
~~~

- Wrong:
~~~{.cpp}
class MyClass {
    ...
};

namespace core {
    ...
}

int main() {
    for (int i = 0; i < 10; ++i) {
        ...
    }

    ...
}
~~~

One-line control clauses should not use braces unless comments are included or a single statement spans multiple lines.

- Right:
~~~{.cpp}
if (condition)
    doIt();

if (condition)
{
    // comment
    doIt();
}

if (condition)
{
    myFunc(reallyLongParam1, reallyLongParam2, ...
        reallyLongParam5);
}
~~~

- Wrong:
~~~{.cpp}
if (condition) {
    doIt();
}

if (condition)
    // comment
    doIt();

if (condition)
    myFunction(reallyLongParam1, reallyLongParam2, ...
        reallyLongParam5);
~~~

Control clauses without a body should use empty braces:

- Right:
~~~{.cpp}
for (; curr; curr = curr->next)
{}
~~~

- Wrong:
~~~{.cpp}
for ( ; curr; curr = curr->next);
~~~


Null, false and 0
-----------------

The null pointer value should be written as `0`.

Tests for true/false, null/non-null, and zero/non-zero should all be done without equality comparisons.

- Right:
~~~{.cpp}
if (condition)
    doIt();

if (!ptr)
    return;

if (!count)
    return;
~~~

- Wrong:
~~~{.cpp}
if (condition == true)
    doIt();

if (ptr == NULL)
    return;

if (count == 0)
    return;
~~~


Floating point literals
-----------------------

Do append `.0`, `.f` and `.0f` to floating point literals.

- Right:
~~~{.cpp}
const double duration = 60.0;

void setDiameter(float diameter)
{
    radius = diameter / 2.0f;
}

setDiameter(10.0f);

const int fps = 12;
double T = 1.0 / fps;
~~~

- Wrong:
~~~{.cpp}
const double duration = 60;

void setDiameter(float diameter)
{
    radius = diameter / 2;
}

setDiameter(10);

const int fps = 12;
double T = 1 / fps; // integer division!
~~~


Names
-----

Use `CamelCase`. Capitalize the first letter in a class, struct, or typedef.
Lower-case the first letter in a namespace, a variable, or function name.

- Right:
~~~{.cpp}
struct Data;
size_t bufferSize;
class HtmlDocument;
String getMimeType();
~~~

- Wrong:
~~~{.cpp}
struct data;
size_t buffer_size;
class HTML_Document;
String getMIMEType();
~~~

Use full words, except in the rare case where an abbreviation would be more canonical and easier to understand.

- Right:
~~~{.cpp}
size_t length;
short tabIndex; // more canonical
~~~

- Wrong:
~~~{.cpp}
size_t len;
short tabulationIndex; // bizarre
~~~

Data members in C++ classes should be private or protected. Static data members should be prefixed by `s_`. Other data members should be prefixed by `m_`.

- Right:
~~~{.cpp}
class String
{
public:
    ...

private:
    short m_length;
};
~~~

- Wrong:
~~~{.cpp}
class String
{
public:
    ...

    short length;
};
~~~

Precede boolean values with words like "is" and "did".

- Right:
~~~{.cpp}
bool isValid;
bool didSendData;
~~~

- Wrong:
~~~{.cpp}
bool valid;
bool sentData;
~~~

Precede setters with the word "set" and getters with the word "get". Setter and getter names should match the names of the variables being set/gotten.

- Right:
~~~{.cpp}
void setCount(size_t); // sets m_count
size_t getCount(); // returns m_count
~~~

- Wrong:
~~~{.cpp}
void count(size_t); // sets m_theCount
size_t count();
~~~

Enum members and `#%define`-d constants should use all uppercase names with words separated by underscores.

Prefer `const` to `#%define`. Prefer `inline` functions to macros.


Other Punctuation
-----------------

Constructors for C++ classes should initialize all of their members using C++ initializer syntax.
Each member (and superclass) should be indented on a separate line, with the colon or comma preceding the member on that line.

- Right:
~~~{.cpp}
MyClass::MyClass(Document* doc)
    : MySuperClass()
    , m_myMember(0)
    , m_doc(doc)
{}

MyOtherClass::MyOtherClass()
    : MySuperClass()
{}
~~~

- Wrong:
~~~{.cpp}
MyClass::MyClass(Document* doc) : MySuperClass()
{
    m_myMember = 0;
    m_doc = doc;
}

MyOtherClass::MyOtherClass() : MySuperClass() {}
~~~

Prefer index over iterator in Vector iterations for a terse, easier-to-read code.

- Right:
~~~{.cpp}
const size_t N = frameViews.size();
for (size_t i = i; i < N; ++i)
    frameViews[i]->update();
~~~

- Wrong:
~~~{.cpp}
const Vector<RefPtr<FrameView> >::iterator end = frameViews.end();
for (Vector<RefPtr<FrameView> >::iterator it = frameViews.begin(); it != end; ++it)
    (*it)->update();
~~~


`#%include` Statements
----------------------

All implementation files must `#%include` the primary header first or second (after PCH file).
So for example, Node.cpp should include Node.h first, before other files.
This guarantees that each header's completeness is tested.
This also assures that each header can be compiled without requiring any other header files be included first.

Includes of system headers must come after includes of other headers.


`using` Statements
------------------

Do not use `using` declarations of any kind to import names in the standard template library. Directly qualify the names at the point they're used instead.


Classes
-------

Use a constructor to do an implicit conversion when the argument is reasonably thought of as a type conversion and the type conversion is fast.
Otherwise, use the `explicit` keyword or a function returning the type. This only applies to single argument constructors.

- Right:
~~~{.cpp}
class LargeInt
{
public:
    LargeInt(int);
...

class Vector
{
public:
    explicit Vector(int size); // not a type conversion.
    PassOwnPtr<Vector> create(Array); // costly conversion.
...
~~~

- Wrong:
~~~{.cpp}
class Task
{
public:
    Task(ScriptExecutionContext*); // not a type conversion.
    explicit Task(); // No arguments.
    explicit Task(ScriptExecutionContext*, Other); // more than one argument.
...
~~~


Comments
--------

Make comments look like sentences by starting with a capital letter and ending with a period (punctation).
One exception may be end of line comments like this "if (x == y) // false for NaN".

Use `FIXME:` or `TODO:` (without attribution) to denote items that need to be addressed in the future.
