// SPDX-License-Identifier: MIT
#pragma once

#include <cassert>
#include <type_traits>
#include <utility>

namespace ocf {

/**
 * @brief A Result type that can represent either a success (Ok) or an error (Err).
 * @tparam T The type of the success value.
 * @tparam E The type of the error value.
 */
template <typename T, typename E>
class Result {
public:
    /**
     * @brief Factory method to create an Ok result.
     * @param value Success value to be stored in the Result.
     * @return Result instance containing the success value.
     */
    static Result Ok(T value) { return Result(std::move(value)); }
    /**
     * @brief Factory method to create an Err result.
     * @param error Error value to be stored in the Result.
     * @return Result instance containing the error value.
     */
    static Result Err(E error) { return Result(std::move(error), Tag::Err); }

    /** Copy and move constructors and assignment operators */
    Result(const Result& other) { copyFrom(other); }

    Result(Result&& other) noexcept { moveFrom(std::move(other)); }

    Result& operator=(const Result& other)
    {
        if (this != &other) {
            destroy();
            copyFrom(other);
        }
        return *this;
    }

    Result& operator=(Result&& other) noexcept
    {
        if (this != &other) {
            destroy();
            moveFrom(std::move(other));
        }
        return *this;
    }

    /**
     * @brief Bool operator to check if the Result is Ok. It will return true if the Result is Ok,
     * and false if it is Err.
     */
    explicit operator bool() const noexcept { return m_isOk; }

    /** Destructor */
    ~Result() { destroy(); }

    /**
     * @brief Whether the Result is Ok or Err.
     * @return If the Result is Ok, returns true. If the Result is Err, returns false.
     */
    [[nodiscard]]
    bool isOk() const noexcept { return m_isOk; }

    /**
     * @brief Whether the Result is Err or Ok.
     * @return If the Result is Err, returns true. If the Result is Ok, returns false.
     */
    [[nodiscard]]
    bool isErr() const noexcept { return !m_isOk; }

    /**
     * @brief Get the success value if the Result is Ok, otherwise it will assert.
     * @return Value stored in the Result if it is Ok.
     */
    T& unwrap() &
    {
        assert(m_isOk);
        return m_value;
    }

    const T& unwrap() const&
    {
        assert(m_isOk);
        return m_value;
    }

    T&& unwrap() &&
    {
        assert(m_isOk);
        return std::move(m_value);
    }

    /**
     * @brief Get the error value if the Result is Err, otherwise it will assert.
     * @return Value stored in the Result if it is Err.
     */
    E& unwrapErr() &
    {
        assert(isErr());
        return m_error;
    }

    /**
     * @brief Get the error value if the Result is Err, otherwise it will assert.
     * @return Const value stored in the Result if it is Err.
     */
    const E& unwrapErr() const&
    {
        assert(isErr());
        return m_error;
    }

    /**
     * @brief Retrieves the values safely.
     * @tparam U Fallback type, which can be implicitly converted to T.
     * @param fallback Fallback value to return if the Result is Err. It will be cast to T if
     * necessary.
     * @return If the result is OK, it returns that value.
     * If there is an error, it returns a fallback value.
     */
    template <typename U>
    T unwrapOr(U&& fallback) const
    {
        return m_isOk ? m_value : static_cast<T>(std::forward<U>(fallback));
    }

private:
    enum class Tag { Ok, Err };

    /** Internal constructor */
    explicit Result(T value)
        : m_isOk(true)
    {
        new (&m_value) T(std::move(value));
    }

    Result(E error, Tag)
        : m_isOk(false)
    {
        new (&m_error) E(std::move(error));
    }

    void destroy()
    {
        if (m_isOk) {
            m_value.~T();
        }
        else {
            m_error.~E();
        }
    }

    void copyFrom(const Result& other)
    {
        m_isOk = other.m_isOk;
        if (m_isOk) {
            new (&m_value) T(other.m_value);
        }
        else {
            new (&m_error) E(other.m_error);
        }
    }

    void moveFrom(Result&& other)
    {
        m_isOk = other.m_isOk;
        if (m_isOk) {
            new (&m_value) T(std::move(other.m_value));
        }
        else {
            new (&m_error) E(std::move(other.m_error));
        }
    }

private:
    bool m_isOk;

    union {
        T m_value;
        E m_error;
    };
};

/**
 * @brief Specialization of Result for void success type.
 * @tparam E The type of the error value.
 */
template <typename E>
class Result<void, E> {
public:
    /**
     * @brief Factory method to create an Ok result.
     * @return Result instance representing a successful result.
     */
    static Result Ok() { return Result(); }

    /**
     * @brief Factory method to create an Err result.
     * @param error Error value to be stored in the Result.
     * @return Result instance containing the error value.
     */
    static Result Err(E error) { return Result(std::move(error), Tag::Err); }

    /** Copy and move constructors and assignment operators */
    Result(const Result& other) { copyFrom(other); }

    Result(Result&& other) noexcept { moveFrom(std::move(other)); }

    Result& operator=(const Result& other)
    {
        if (this != &other) {
            destroy();
            copyFrom(other);
        }
        return *this;
    }

    Result& operator=(Result&& other) noexcept
    {
        if (this != &other) {
            destroy();
            moveFrom(std::move(other));
        }
        return *this;
    }

    /**
     * @brief Bool operator to check if the Result is Ok. It will return true if the Result is Ok,
     * and false if it is Err.
     */
    explicit operator bool() const noexcept { return m_isOk; }

    /** Destructor */
    ~Result() { destroy(); }

    /**
     * @brief Whether the Result is Ok or Err.
     * @return If the Result is Ok, returns true. If the Result is Err, returns false.
     */
    [[nodiscard]]
    bool isOk() const noexcept
    {
        return m_isOk;
    }

    /**
     * @brief Whether the Result is Err or Ok.
     * @return If the Result is Err, returns true. If the Result is Ok, returns false.
     */
    [[nodiscard]]
    bool isErr() const noexcept
    {
        return !m_isOk;
    }

    /**
     * @brief Get the success value if the Result is Ok, otherwise it will assert.
     * @return Value stored in the Result if it is Ok.
     */
    void unwrap() const { assert(m_isOk && "called unwrap on Err"); }

    /**
     * @brief Get the error value if the Result is Err, otherwise it will assert.
     * @return Value stored in the Result if it is Err.
     */
    E& unwrapErr() &
    {
        assert(isErr());
        return m_error;
    }

    const E& unwrapErr() const&
    {
        assert(isErr());
        return m_error;
    }

    E&& unwrapErr() &&
    {
        assert(isErr());
        return std::move(m_error);
    }

    /**
     * @brief Expect the Result to be Ok, otherwise it will assert with the provided message.
     * @param msg Display message when the assertion fails.
     */
    void expect(const char* msg) const
    {
        if (!m_isOk) {
            assert(false && msg);
        }
    }

private:
    enum class Tag { Ok, Err };

    /** Internal constructor */
    Result() noexcept
        : m_isOk(true)
    {
    }

    Result(E error, Tag)
        : m_isOk(false)
    {
        new (&m_error) E(std::move(error));
    }

    void destroy()
    {
        if (!m_isOk) {
            m_error.~E();
        }
    }

    void copyFrom(const Result& other)
    {
        m_isOk = other.m_isOk;
        if (!m_isOk) {
            new (&m_error) E(other.m_error);
        }
    }

    void moveFrom(Result&& other)
    {
        m_isOk = other.m_isOk;
        if (!m_isOk) {
            new (&m_error) E(std::move(other.m_error));
        }
    }

private:
    bool m_isOk;

    union {
        E m_error;
    };
};

} // namespace ocf
