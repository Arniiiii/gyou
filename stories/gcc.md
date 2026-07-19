# The project is not compilable with GCC

I wanted to write a little bit more safe code with const correctness and etc. Also, I already once had a memory bug because of it.

So, what is the problem?

I wanted to have something like this from Rust: 

```rust
let something = {  expr.await?  };
```

In C++, the best way to describe it using only standard construction is:
```cpp
std::expected<a_value_type, an_error_type> do_not_use_this = co_await expr;
if (not do_not_use_this)
    {
        // if it wasn't `co_await`, we wouldn't be in coroutine context and thus
        // would use `return`
        co_return std::unexpected(std::move(do_not_use_this.error()));
    }
a_value_type const something = std::move(do_not_use_this.value());
```

As it is prominently featured, the variable `do_not_use_this` is actually visible. Furthermore, too much boilerplate for this.

The solution I found is GNU statement expression extension. Basically it is next:

```cpp
auto const something = ({
    auto tmp;
    ...
    tmp;
});
```

Here's an example of usage in the project:
```cpp
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) */
#define TRY_OR_CO_RETURN(expr)                                      \
    __extension__({                                                 \
        auto&& _res = (expr);                                       \
        if (!_res)                                                  \
            {                                                       \
                co_return std::unexpected(std::move(_res.error())); \
            }                                                       \
        std::move(*_res);                                           \
    })

// inside a coroutine:
EbuildSpecificData const ebuild_data = TRY_OR_CO_RETURN(
    co_await get_ebuild_info(ioc, cfg, common_ctx, path_to_ebuild));
```



Great. 

But here's the caveat: 

This code does not compile with GCC in form of giving ICE (internal compiler error). 

But it compiles well with Clang. 

Why?

Somehow related to the use of `co_return` inside the extension.

That's funny: GCC can't compile its own extension.

Anyway, more details at next links:

https://github.com/Arniiiii/gyou/issues/1

https://bugs.gentoo.org/977697

https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125997

https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121016
