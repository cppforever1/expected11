#ifndef EXPECTED11_EXPECTED11_HPP
#define EXPECTED11_EXPECTED11_HPP

#include <cstddef>
#include <exception>
#include <new>
#include <type_traits>
#include <utility>

namespace expected11
{

    class SourceLocation
    {
    public:
        SourceLocation() : file_name_(""), function_name_(""), line_(0) {}
        SourceLocation(const char *file_name, const char *function_name,
                       std::size_t line)
            : file_name_(file_name), function_name_(function_name), line_(line) {}

        const char *file_name() const { return file_name_; }
        const char *function_name() const { return function_name_; }
        std::size_t line() const { return line_; }

    private:
        const char *file_name_;
        const char *function_name_;
        std::size_t line_;
    };

#define EXPECTED11_SOURCE_LOCATION() \
    ::expected11::SourceLocation(__FILE__, __FUNCTION__, static_cast<std::size_t>(__LINE__))

    template <typename E>
    class unexpected
    {
    public:
        typedef E error_type;

        explicit unexpected(const E &error, SourceLocation location = SourceLocation())
            : error_(error), source_location_(location) {}
        explicit unexpected(E &&error, SourceLocation location = SourceLocation())
            : error_(std::move(error)), source_location_(location) {}
        template <typename U>
        explicit unexpected(const U &error,
                            typename std::enable_if<std::is_constructible<E, const U &>::value, int>::type = 0)
            : error_(error), source_location_() {}

        E &error() & { return error_; }
        const E &error() const & { return error_; }
        E &&error() && { return std::move(error_); }
        const E &&error() const && { return std::move(error_); }
        const SourceLocation &source_location() const { return source_location_; }

    private:
        E error_;
        SourceLocation source_location_;
    };

    template <typename E>
    unexpected<typename std::decay<E>::type> make_unexpected(E &&error)
    {
        return unexpected<typename std::decay<E>::type>(std::forward<E>(error));
    }

    template <typename E>
    unexpected<typename std::decay<E>::type> make_unexpected(
        E &&error, SourceLocation location)
    {
        return unexpected<typename std::decay<E>::type>(std::forward<E>(error), location);
    }

#define EXPECTED11_MAKE_UNEXPECTED(error) \
    ::expected11::make_unexpected((error), EXPECTED11_SOURCE_LOCATION())

    struct unexpect_t
    {
        explicit unexpect_t() {}
    };
    static const unexpect_t unexpect = unexpect_t();

    struct in_place_t
    {
        explicit in_place_t() {}
    };
    static const in_place_t in_place = in_place_t();

    template <typename E>
    class bad_expected_access : public std::exception
    {
    public:
        explicit bad_expected_access(E error) : error_(std::move(error)) {}
        const char *what() const throw() { return "bad expected access"; }
        E &error() { return error_; }
        const E &error() const { return error_; }

    private:
        E error_;
    };

    namespace detail
    {
        template <typename T, typename E>
        struct storage
        {
            union data
            {
                char empty;
                T value;
                E error;
                data() : empty() {}
                ~data() {}
            } data;
            bool has_value;
            bool engaged;
            SourceLocation source_location;
            storage() : data(), has_value(false), engaged(false), source_location() {}
            ~storage() {}
        };

    }

    template <typename T, typename E>
    class expected
    {
    public:
        typedef T value_type;
        typedef E error_type;

        expected() : storage_()
        {
            new (&storage_.data.value) T();
            storage_.has_value = true;
            storage_.engaged = true;
        }
        expected(const T &value) : storage_()
        {
            new (&storage_.data.value) T(value);
            storage_.has_value = true;
            storage_.engaged = true;
        }
        expected(T &&value) : storage_()
        {
            new (&storage_.data.value) T(std::move(value));
            storage_.has_value = true;
            storage_.engaged = true;
        }
        expected(const unexpected<E> &error) : storage_()
        {
            new (&storage_.data.error) E(error.error());
            storage_.source_location = error.source_location();
            storage_.engaged = true;
        }
        expected(unexpected<E> &&error) : storage_()
        {
            storage_.source_location = error.source_location();
            new (&storage_.data.error) E(std::move(error).error());
            storage_.engaged = true;
        }
        template <typename U, typename G>
        expected(const expected<U, G> &other,
                 typename std::enable_if<std::is_constructible<T, const U &>::value &&
                                             std::is_constructible<E, const G &>::value,
                                         int>::type = 0)
            : storage_()
        {
            if (other.has_value())
            {
                new (&storage_.data.value) T(other.value());
                storage_.has_value = true;
            }
            else
            {
                new (&storage_.data.error) E(other.error());
                storage_.source_location = other.source_location();
            }
            storage_.engaged = true;
        }
        template <typename U, typename G>
        expected(expected<U, G> &&other,
                 typename std::enable_if<std::is_constructible<T, U &&>::value &&
                                             std::is_constructible<E, G &&>::value,
                                         int>::type = 0)
            : storage_()
        {
            if (other.has_value())
            {
                new (&storage_.data.value) T(std::move(other).value());
                storage_.has_value = true;
            }
            else
            {
                storage_.source_location = other.source_location();
                new (&storage_.data.error) E(std::move(other).error());
            }
            storage_.engaged = true;
        }
        template <typename G>
        expected(const unexpected<G> &error,
                 typename std::enable_if<std::is_constructible<E, const G &>::value,
                                         int>::type = 0)
            : storage_()
        {
            new (&storage_.data.error) E(error.error());
            storage_.source_location = error.source_location();
            storage_.engaged = true;
        }
        template <typename G>
        expected(unexpected<G> &&error,
                 typename std::enable_if<std::is_constructible<E, G &&>::value,
                                         int>::type = 0)
            : storage_()
        {
            storage_.source_location = error.source_location();
            new (&storage_.data.error) E(std::move(error).error());
            storage_.engaged = true;
        }

        template <typename... Args>
        explicit expected(in_place_t, Args &&...args) : storage_()
        {
            new (&storage_.data.value) T(std::forward<Args>(args)...);
            storage_.has_value = true;
            storage_.engaged = true;
        }

        template <typename... Args>
        explicit expected(unexpect_t, Args &&...args) : storage_()
        {
            new (&storage_.data.error) E(std::forward<Args>(args)...);
            storage_.engaged = true;
        }

        expected(const expected &other) : storage_()
        {
            if (other.has_value())
            {
                new (&storage_.data.value) T(other.value());
                storage_.has_value = true;
                storage_.engaged = true;
            }
            else
            {
                new (&storage_.data.error) E(other.error());
                storage_.source_location = other.source_location();
                storage_.engaged = true;
            }
        }
        expected(expected &&other) : storage_()
        {
            if (other.has_value())
            {
                new (&storage_.data.value) T(std::move(other).value());
                storage_.has_value = true;
                storage_.engaged = true;
            }
            else
            {
                new (&storage_.data.error) E(std::move(other).error());
                storage_.source_location = other.source_location();
                storage_.engaged = true;
            }
        }
        ~expected() { destroy(); }

        expected &operator=(const expected &other)
        {
            if (this != &other) { expected replacement(other); assign_from(std::move(replacement)); }
            return *this;
        }
        expected &operator=(expected &&other)
        {
            if (this != &other) { expected replacement(std::move(other)); assign_from(std::move(replacement)); }
            return *this;
        }
        expected &operator=(const T &value)
        {
            expected replacement(value);
            assign_from(std::move(replacement));
            return *this;
        }
        expected &operator=(T &&value)
        {
            expected replacement(std::move(value));
            assign_from(std::move(replacement));
            return *this;
        }
        expected &operator=(const unexpected<E> &error)
        {
            expected replacement(error);
            assign_from(std::move(replacement));
            return *this;
        }
        expected &operator=(unexpected<E> &&error)
        {
            expected replacement(std::move(error));
            assign_from(std::move(replacement));
            return *this;
        }

        bool has_value() const { return storage_.engaged && storage_.has_value; }
        explicit operator bool() const { return has_value(); }
        const SourceLocation &source_location() const { return storage_.source_location; }
        T &value() &
        {
            check_value();
            return storage_.data.value;
        }
        const T &value() const &
        {
            check_value();
            return storage_.data.value;
        }
        T &&value() &&
        {
            check_value();
            return std::move(storage_.data.value);
        }
        const T &&value() const &&
        {
            check_value();
            return std::move(storage_.data.value);
        }
        E &error() & { return storage_.data.error; }
        const E &error() const & { return storage_.data.error; }
        E &&error() && { return std::move(storage_.data.error); }
        const E &&error() const && { return std::move(storage_.data.error); }
        T &operator*() { return value(); }
        const T &operator*() const { return value(); }
        T *operator->() { return &value(); }
        const T *operator->() const { return &value(); }
        T value_or(T fallback) const { return has_value() ? value() : fallback; }
        E error_or(E fallback) const { return has_value() ? fallback : error(); }

        void emplace(const T &value)
        {
            expected replacement(value);
            swap(replacement);
        }
        void emplace(T &&value)
        {
            expected replacement(std::move(value));
            swap(replacement);
        }
        template <typename... Args>
        void emplace(Args &&...args)
        {
            expected replacement(in_place, std::forward<Args>(args)...);
            swap(replacement);
        }
        void swap(expected &other)
        {
            if (has_value() && other.has_value())
            {
                using std::swap;
                swap(value(), other.value());
            }
            else if (!has_value() && !other.has_value())
            {
                using std::swap;
                swap(error(), other.error());
            }
            else
            {
                expected temp(std::move(other));
                other = std::move(*this);
                *this = std::move(temp);
            }
        }

        template <typename F>
        typename std::result_of<F(T &)>::type and_then(F function) { return has_value() ? function(value()) : typename std::result_of<F(T &)>::type(make_unexpected(error(), source_location())); }
        template <typename F>
        typename std::result_of<F(const T &)>::type and_then(F function) const { return has_value() ? function(value()) : typename std::result_of<F(const T &)>::type(make_unexpected(error(), source_location())); }
        template <typename F>
        expected<typename std::result_of<F(T &)>::type, E> transform(F function) { return has_value() ? expected<typename std::result_of<F(T &)>::type, E>(function(value())) : expected<typename std::result_of<F(T &)>::type, E>(make_unexpected(error(), source_location())); }
        template <typename F>
        expected<typename std::result_of<F(const T &)>::type, E> transform(F function) const { return has_value() ? expected<typename std::result_of<F(const T &)>::type, E>(function(value())) : expected<typename std::result_of<F(const T &)>::type, E>(make_unexpected(error(), source_location())); }
        template <typename F>
        expected<T, typename std::result_of<F(E &)>::type> transform_error(F function) { return has_value() ? expected<T, typename std::result_of<F(E &)>::type>(value()) : expected<T, typename std::result_of<F(E &)>::type>(make_unexpected(function(error()), source_location())); }
        template <typename F>
        expected<T, typename std::result_of<F(const E &)>::type> transform_error(F function) const { return has_value() ? expected<T, typename std::result_of<F(const E &)>::type>(value()) : expected<T, typename std::result_of<F(const E &)>::type>(make_unexpected(function(error()), source_location())); }
        template <typename F>
        typename std::result_of<F(E &)>::type or_else(F function) { typedef typename std::result_of<F(E &)>::type result_type; return has_value() ? result_type(value()) : function(error()); }
        template <typename F>
        typename std::result_of<F(const E &)>::type or_else(F function) const { typedef typename std::result_of<F(const E &)>::type result_type; return has_value() ? result_type(value()) : function(error()); }

    private:
        void assign_from(expected &&other)
        {
            destroy();
            if (other.has_value())
            {
                new (&storage_.data.value) T(std::move(other).value());
                storage_.has_value = true;
                storage_.source_location = SourceLocation();
            }
            else
            {
                storage_.source_location = other.source_location();
                new (&storage_.data.error) E(std::move(other).error());
            }
            storage_.engaged = true;
        }
        void check_value() const
        {
            if (!has_value())
                throw bad_expected_access<E>(error());
        }
        void destroy()
        {
            if (!storage_.engaged)
                return;
            if (storage_.has_value)
                storage_.data.value.~T();
            else
                storage_.data.error.~E();
            storage_.has_value = false;
            storage_.engaged = false;
        }
        detail::storage<T, E> storage_;
    };

    template <typename E>
    class expected<void, E>
    {
    public:
        typedef void value_type;
        typedef E error_type;
        expected() : storage_(), has_value_(true), engaged_(true), source_location_() {}
        expected(const unexpected<E> &error) : storage_(), has_value_(false), engaged_(false), source_location_(error.source_location())
        {
            new (&storage_.error) E(error.error());
            engaged_ = true;
        }
        expected(unexpected<E> &&error) : storage_(), has_value_(false), engaged_(false), source_location_(error.source_location())
        {
            new (&storage_.error) E(std::move(error).error());
            engaged_ = true;
        }
        template <typename G>
        expected(const unexpected<G> &error,
                 typename std::enable_if<std::is_constructible<E, const G &>::value, int>::type = 0)
            : storage_(), has_value_(false), engaged_(false), source_location_(error.source_location())
        {
            new (&storage_.error) E(error.error());
            engaged_ = true;
        }
        template <typename G>
        expected(unexpected<G> &&error,
                 typename std::enable_if<std::is_constructible<E, G &&>::value, int>::type = 0)
            : storage_(), has_value_(false), engaged_(false), source_location_(error.source_location())
        {
            new (&storage_.error) E(std::move(error).error());
            engaged_ = true;
        }
        template <typename G>
        expected(const expected<void, G> &other,
                 typename std::enable_if<std::is_constructible<E, const G &>::value, int>::type = 0)
            : storage_(), has_value_(other.has_value()), engaged_(false), source_location_(other.source_location())
        {
            if (!has_value_)
                new (&storage_.error) E(other.error());
            engaged_ = true;
        }
        template <typename G>
        expected(expected<void, G> &&other,
                 typename std::enable_if<std::is_constructible<E, G &&>::value, int>::type = 0)
            : storage_(), has_value_(other.has_value()), engaged_(false), source_location_(other.source_location())
        {
            if (!has_value_)
                new (&storage_.error) E(std::move(other).error());
            engaged_ = true;
        }
        template <typename... Args>
        explicit expected(unexpect_t, Args &&...args) : storage_(), has_value_(false), engaged_(false), source_location_()
        {
            new (&storage_.error) E(std::forward<Args>(args)...);
            engaged_ = true;
        }
        expected(const expected &other) : storage_(), has_value_(other.has_value_), engaged_(false), source_location_(other.source_location_)
        {
            if (!has_value_)
                new (&storage_.error) E(other.error());
            engaged_ = true;
        }
        expected(expected &&other) : storage_(), has_value_(other.has_value_), engaged_(false), source_location_(other.source_location_)
        {
            if (!has_value_)
                new (&storage_.error) E(std::move(other).error());
            engaged_ = true;
        }
        ~expected()
        {
            if (engaged_ && !has_value_)
                storage_.error.~E();
        }
        expected &operator=(const expected &other)
        {
            if (this != &other) { expected replacement(other); assign_from(std::move(replacement)); }
            return *this;
        }
        expected &operator=(expected &&other)
        {
            if (this != &other) { expected replacement(std::move(other)); assign_from(std::move(replacement)); }
            return *this;
        }
        expected &operator=(const unexpected<E> &error)
        {
            expected replacement(error);
            assign_from(std::move(replacement));
            return *this;
        }
        expected &operator=(unexpected<E> &&error)
        {
            expected replacement(std::move(error));
            assign_from(std::move(replacement));
            return *this;
        }
        bool has_value() const { return engaged_ && has_value_; }
        explicit operator bool() const { return has_value(); }
        const SourceLocation &source_location() const { return source_location_; }
        void value() const
        {
            if (!has_value_)
                throw bad_expected_access<E>(error());
        }
        E &error() { return storage_.error; }
        const E &error() const { return storage_.error; }
        E error_or(E fallback) const { return has_value_ ? fallback : error(); }

        template <typename F>
        typename std::result_of<F()>::type and_then(F function)
        {
            typedef typename std::result_of<F()>::type result_type;
            return has_value() ? function() : result_type(make_unexpected(error(), source_location()));
        }
        template <typename F>
        typename std::result_of<F()>::type and_then(F function) const
        {
            typedef typename std::result_of<F()>::type result_type;
            return has_value() ? function() : result_type(make_unexpected(error(), source_location()));
        }
        template <typename F>
        expected<typename std::result_of<F()>::type, E> transform(F function)
        {
            typedef typename std::result_of<F()>::type result_type;
            return has_value() ? transform_success<result_type>(function, std::is_void<result_type>())
                               : expected<result_type, E>(make_unexpected(error(), source_location()));
        }
        template <typename F>
        expected<typename std::result_of<F()>::type, E> transform(F function) const
        {
            typedef typename std::result_of<F()>::type result_type;
            return has_value() ? transform_success<result_type>(function, std::is_void<result_type>())
                               : expected<result_type, E>(make_unexpected(error(), source_location()));
        }
        template <typename F>
        expected<void, typename std::result_of<F(E &)>::type> transform_error(F function)
        {
            typedef typename std::result_of<F(E &)>::type error_type;
            return has_value() ? expected<void, error_type>()
                               : expected<void, error_type>(make_unexpected(function(error()), source_location()));
        }
        template <typename F>
        expected<void, typename std::result_of<F(const E &)>::type> transform_error(F function) const
        {
            typedef typename std::result_of<F(const E &)>::type error_type;
            return has_value() ? expected<void, error_type>()
                               : expected<void, error_type>(make_unexpected(function(error()), source_location()));
        }
        template <typename F>
        typename std::result_of<F(E &)>::type or_else(F function)
        {
            typedef typename std::result_of<F(E &)>::type result_type;
            return has_value() ? result_type() : function(error());
        }
        template <typename F>
        typename std::result_of<F(const E &)>::type or_else(F function) const
        {
            typedef typename std::result_of<F(const E &)>::type result_type;
            return has_value() ? result_type() : function(error());
        }

        void emplace()
        {
            reset();
            has_value_ = true;
            engaged_ = true;
            source_location_ = SourceLocation();
        }
        void swap(expected &other)
        {
            if (has_value_ && other.has_value_)
                return;
            if (!has_value_ && !other.has_value_)
            {
                using std::swap;
                swap(error(), other.error());
                return;
            }
            expected temp(std::move(other));
            other = std::move(*this);
            *this = std::move(temp);
        }

    private:
        void assign_from(expected &&other)
        {
            reset();
            has_value_ = other.has_value_;
            source_location_ = other.source_location_;
            if (!has_value_)
                new (&storage_.error) E(std::move(other).error());
            engaged_ = true;
        }
        union storage
        {
            char empty;
            E error;
            storage() : empty() {}
            ~storage() {}
        } storage_;
        bool has_value_;
        bool engaged_;
        SourceLocation source_location_;
        template <typename R, typename F>
        expected<R, E> transform_success(F function, std::false_type) const
        {
            return expected<R, E>(function());
        }
        template <typename R, typename F>
        expected<R, E> transform_success(F function, std::true_type) const
        {
            function();
            return expected<R, E>();
        }
        void reset()
        {
            if (engaged_ && !has_value_)
                storage_.error.~E();
            engaged_ = false;
            has_value_ = false;
        }
    };

    template <typename E>
    bool operator==(const unexpected<E> &left, const unexpected<E> &right)
    {
        return left.error() == right.error();
    }

    template <typename E>
    bool operator!=(const unexpected<E> &left, const unexpected<E> &right)
    {
        return !(left == right);
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator==(const expected<T, E> &left, const expected<T, E> &right)
    {
        if (left.has_value() != right.has_value())
            return false;
        return left.has_value() ? left.value() == right.value() : left.error() == right.error();
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator!=(const expected<T, E> &left, const expected<T, E> &right)
    {
        return !(left == right);
    }

    template <typename E>
    bool operator==(const expected<void, E> &left, const expected<void, E> &right)
    {
        if (left.has_value() != right.has_value())
            return false;
        return left.has_value() || left.error() == right.error();
    }

    template <typename E>
    bool operator!=(const expected<void, E> &left, const expected<void, E> &right)
    {
        return !(left == right);
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator==(const expected<T, E> &value, const unexpected<E> &error)
    {
        return !value.has_value() && value.error() == error.error();
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator==(const unexpected<E> &error, const expected<T, E> &value)
    {
        return value == error;
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator!=(const expected<T, E> &value, const unexpected<E> &error)
    {
        return !(value == error);
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator!=(const unexpected<E> &error, const expected<T, E> &value)
    {
        return !(error == value);
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator==(const expected<T, E> &value, const T &expected_value)
    {
        return value.has_value() && value.value() == expected_value;
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator==(const T &expected_value, const expected<T, E> &value)
    {
        return value == expected_value;
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator!=(const expected<T, E> &value, const T &expected_value)
    {
        return !(value == expected_value);
    }

    template <typename T, typename E>
    typename std::enable_if<!std::is_void<T>::value, bool>::type
    operator!=(const T &expected_value, const expected<T, E> &value)
    {
        return !(expected_value == value);
    }

    template <typename T, typename E>
    void swap(expected<T, E> &left, expected<T, E> &right) { left.swap(right); }

    template <typename E>
    void swap(expected<void, E> &left, expected<void, E> &right) { left.swap(right); }

} // namespace expected11

#endif
