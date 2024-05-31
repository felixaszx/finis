#ifndef INCLUDE_TOOLS_HPP
#define INCLUDE_TOOLS_HPP

#define bit_shift_left(bits) (1 << bits)
#define TRY_FUNC \
    try          \
    {

#define CATCH_BEGIN            \
    }                          \
    catch (std::exception & e) \
    {                          \
        std::cerr << e.what();
#define CATCH_END }
#define CATCH_FUNC             \
    }                          \
    catch (std::exception & e) \
    {                          \
        std::cerr << e.what(); \
    }

#endif // INCLUDE_TOOLS_HPP
