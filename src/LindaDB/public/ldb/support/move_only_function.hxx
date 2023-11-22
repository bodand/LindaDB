/* LindaDB project
 *
 * Originally created: 2023-11-08.
 *
 * src/LindaRT/include/move_only_function --
 *   Backport of std::move_only_function to C++20 from MS STL.
 */
#ifndef LINDADB_MOVE_ONLY_FUNCTION_HXX
#define LINDADB_MOVE_ONLY_FUNCTION_HXX

#include <functional>
#include <type_traits>

#if defined(__GNUG__) || defined(__clang__)
#  pragma GCC system_header
#endif

namespace ldb {

    template<class>
    struct fail : std::false_type { };

    // this branch of the code is directly taken from https://github.com/microsoft/STL
    // their copyright applies: https://github.com/microsoft/STL/blob/main/LICENSE.txt
    // modified in a way to be usable from "user-code"
#ifndef __cpp_lib_move_only_function
#  ifdef __GNUG__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#  endif

    template<class Type, template<class...> class Template>
    inline constexpr bool Is_specialization_v = false; // true if and only if Type is a specialization of Template
    template<template<class...> class Template, class... Types>
    inline constexpr bool Is_specialization_v<Template<Types...>, Template> = true;

    template<class Type, template<class...> class Template>
    struct Is_specialization : std::bool_constant<Is_specialization_v<Type, Template>> { };

    inline constexpr int Small_object_num_ptrs = 6 + 16 / sizeof(void*);

    // Move_only_function_data is defined as an array of pointers.
    // The first element is always a pointer to Move_only_function_base::Impl_t; it emulates a vtable pointer.
    // The other pointers are used as storage for a small functor;
    // if the functor does not fit in, the second pointer is the pointer to allocated storage, the rest are unused.
    union alignas(max_align_t) Move_only_function_data {
        void* Pointers[Small_object_num_ptrs];
        const void* Impl;
        char Data; // For aliasing

        template<class Fn>
        static constexpr size_t Buf_offset =
               alignof(Fn) <= sizeof(Impl) ? sizeof(Impl) // Store Fn immediately after Impl
                                           : alignof(Fn); // Pad Fn to next alignment

        template<class Fn>
        static constexpr size_t Buf_size = sizeof(Pointers) - Buf_offset<Fn>;

        template<class Fn>
        [[nodiscard]] void*
        Buf_ptr() noexcept {
            return &Data + Buf_offset<Fn>;
        }

        template<class Fn>
        [[nodiscard]] Fn*
        Small_fn_ptr() const noexcept {
            // cast away const to avoid complication of const propagation to here;
            // const correctness is still enforced by Move_only_function_call specializations.
            return static_cast<Fn*>(const_cast<Move_only_function_data*>(this)->Buf_ptr<Fn>());
        }

        template<class Fn>
        [[nodiscard]] Fn*
        Large_fn_ptr() const noexcept {
            return static_cast<Fn*>(Pointers[1]);
        }

        void
        Set_large_fn_ptr(void* const Value) noexcept {
            Pointers[1] = Value;
        }
    };

    // Size of a large function. Treat an empty function as if it has this size.
    // Treat a small function as if it has this size too if it fits and is trivially copyable.
    inline constexpr size_t Minimum_function_size = 2 * sizeof(void*);

    // The below functions are __stdcall as they are called by pointers from Move_only_function_base::Impl_t.
    // (We use explicit __stdcall to make the ABI stable for translation units with different calling convention options.)
    // Non-template functions are still defined inline, as the compiler may be able to devirtualize some calls.

    template<class Rx, class... Types>
    [[noreturn]] Rx __stdcall Function_not_callable(const Move_only_function_data&, Types&&...) noexcept {
        abort(); // Unlike std::function, move_only_function doesn't throw bad_function_call
                 // (N4950 [func.wrap.move.inv]/2)
    }

    template<class Vt, class VtInvQuals, class Rx, bool Noex, class... Types>
    [[nodiscard]] Rx __stdcall Function_inv_small(const Move_only_function_data& Self, Types&&... Args) noexcept(Noex) {
        if constexpr (std::is_void_v<Rx>) {
            (void) std::invoke(static_cast<VtInvQuals>(*Self.Small_fn_ptr<Vt>()), std::forward<Types>(Args)...);
        }
        else {
            return std::invoke(static_cast<VtInvQuals>(*Self.Small_fn_ptr<Vt>()), std::forward<Types>(Args)...);
        }
    }

    template<class Vt, class VtInvQuals, class Rx, bool Noex, class... Types>
    [[nodiscard]] Rx __stdcall Function_inv_large(const Move_only_function_data& Self, Types&&... Args) noexcept(Noex) {
        if constexpr (std::is_void_v<Rx>) {
            (void) std::invoke(static_cast<VtInvQuals>(*Self.Large_fn_ptr<Vt>()), std::forward<Types>(Args)...);
        }
        else {
            return std::invoke(static_cast<VtInvQuals>(*Self.Large_fn_ptr<Vt>()), std::forward<Types>(Args)...);
        }
    }

    template<class Vt>
    void __stdcall Function_move_small(Move_only_function_data& Self, Move_only_function_data& Src) noexcept {
        const auto Src_fn_ptr = Src.Small_fn_ptr<Vt>();
        ::new (Self.Buf_ptr<Vt>()) Vt(std::move(*Src_fn_ptr));
        Src_fn_ptr->~Vt();
        Self.Impl = Src.Impl;
    }

    template<size_t Size>
    void __stdcall Function_move_memcpy(Move_only_function_data& Self, Move_only_function_data& Src) noexcept {
        memcpy(&Self.Data, &Src.Data, Size); // Copy Impl* and functor data
    }

    inline void __stdcall Function_move_large(Move_only_function_data& Self, Move_only_function_data& Src) noexcept {
        memcpy(&Self.Data, &Src.Data, Minimum_function_size); // Copy Impl* and functor data
    }

    template<class Vt>
    void __stdcall Function_destroy_small(Move_only_function_data& Self) noexcept {
        Self.Small_fn_ptr<Vt>()->~Vt();
    }

    inline void __stdcall Function_deallocate_large_default_aligned(Move_only_function_data& Self) noexcept {
        ::operator delete(Self.Large_fn_ptr<void>());
    }

    template<size_t Align>
    void __stdcall Function_deallocate_large_overaligned(Move_only_function_data& Self) noexcept {
        static_assert(Align > __STDCPP_DEFAULT_NEW_ALIGNMENT__);
#  ifdef __cpp_aligned_new
        ::operator delete(Self.Large_fn_ptr<void>(), std::align_val_t{Align});
#  else  // ^^^ defined(__cpp_aligned_new) / !defined(__cpp_aligned_new) vvv
        ::operator delete(Self.Large_fn_ptr<void>());
#  endif // ^^^ !defined(__cpp_aligned_new) ^^^
    }

    template<class Vt>
    void __stdcall Function_destroy_large(Move_only_function_data& Self) noexcept {
        const auto Pfn = Self.Large_fn_ptr<Vt>();
        Pfn->~Vt();
#  ifdef __cpp_aligned_new
        if constexpr (alignof(Vt) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
            ::operator delete(static_cast<void*>(Pfn), std::align_val_t{alignof(Vt)});
        }
        else
#  endif // defined(__cpp_aligned_new)
        {
            ::operator delete(static_cast<void*>(Pfn));
        }
    }

    template<class Vt>
    inline constexpr size_t Function_small_copy_size =                              // We copy Impl* and the functor data at once
           Move_only_function_data::Buf_offset<Vt>                                  // Impl* plus possible alignment
           + (size_t{sizeof(Vt) + sizeof(void*) - 1} & ~size_t{sizeof(void*) - 1}); // size in whole pointers

    template<class Vt, class... CTypes>
    [[nodiscard]] void*
    Function_new_large(CTypes&&... Args) {
        struct [[nodiscard]] Guard_type {
            void* Ptr;

            ~
            Guard_type() {
                // Ptr is not nullptr only if an exception is thrown as a result of Vt construction.
                // Check Ptr before calling operator delete to save the call in the common case.
                if (Ptr) {
#  ifdef __cpp_aligned_new
                    if constexpr (alignof(Vt) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
                        ::operator delete(Ptr, std::align_val_t{alignof(Vt)});
                    }
                    else
#  endif // defined(__cpp_aligned_new)
                    {
                        ::operator delete(Ptr);
                    }
                }
            }
        };

        void* Ptr;
#  ifdef __cpp_aligned_new
        if constexpr (alignof(Vt) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
            Ptr = ::operator new(sizeof(Vt), std::align_val_t{alignof(Vt)});
        }
        else
#  endif // defined(__cpp_aligned_new)
        {
            Ptr = ::operator new(sizeof(Vt));
        }
        Guard_type Guard{Ptr};
        ::new (Ptr) Vt(std::forward<CTypes>(Args)...);
        Guard.Ptr = nullptr;
        return Ptr;
    }

    template<class Rx, bool Noexcept, class... Types>
    class Move_only_function_base {
    public:
        // TRANSITION, DevCom-1208330: use noexcept(Noexcept) instead
        template<bool>
        struct Invoke_t {
            using Call = Rx(__stdcall*)(const Move_only_function_data&, Types&&...);
        };

        template<>
        struct Invoke_t<true> {
            using Call = Rx(__stdcall*)(const Move_only_function_data&, Types&&...) noexcept;
        };

        struct Impl_t { // A per-callable-type structure acting as a virtual function table.
            // Using vtable emulations gives more flexibility for optimizations and reduces the amount of RTTI data.
            // (The RTTI savings may be significant as with lambdas and binds there may be many distinct callable types.
            // Here we don't have a distinct wrapper class for each callable type, only distinct functions when needed.)

            // Move and Destroy are nullptr if trivial. Besides being an optimization, this enables assigning an
            // empty function from a DLL that is unloaded later, and then safely moving/destroying that empty function.

            // Calls target
            Invoke_t<Noexcept>::Call Invoke;
            // Moves the data, including pointer to "vtable", AND destroys old data (not resetting its "vtable").
            // nullptr if we can trivially move two pointers.
            void(__stdcall* Move)(Move_only_function_data&, Move_only_function_data&) noexcept;
            // Destroys data (not resetting its "vtable").
            // nullptr if destruction is a no-op.
            void(__stdcall* Destroy)(Move_only_function_data&) noexcept;
        };

        Move_only_function_data Data;

        Move_only_function_base() noexcept = default; // leaves fields uninitialized

        Move_only_function_base(Move_only_function_base&& Other) noexcept {
            Checked_move(Data, Other.Data);
            Other.Reset_to_null();
        }

        void
        Construct_with_null() noexcept {
            Data.Impl = nullptr;
            Data.Set_large_fn_ptr(nullptr); // std::initialize, since we'll be copying it
        }

        void
        Reset_to_null() noexcept {
            Data.Impl = nullptr;
        }

        template<class Vt, class VtInvQuals, class... CTypes>
        void
        Construct_with_fn(CTypes&&... Args) {
            Data.Impl = Create_impl_ptr<Vt, VtInvQuals>();
            if constexpr (Large_function_engaged<Vt>) {
                Data.Set_large_fn_ptr(Function_new_large<Vt>(std::forward<CTypes>(Args)...));
            }
            else {
                ::new (Data.Buf_ptr<Vt>()) Vt(std::forward<CTypes>(Args)...);
            }
        }

        static void
        Checked_destroy(Move_only_function_data& Data) noexcept {
            const auto Impl = Get_impl(Data);
            if (Impl->Destroy) {
                Impl->Destroy(Data);
            }
        }

        static void
        Checked_move(Move_only_function_data& Data, Move_only_function_data& Src) noexcept {
            const auto Impl = Get_impl(Src);
            if (Impl->Move) {
                Impl->Move(Data, Src);
            }
            else {
                Function_move_large(Data, Src);
            }
        }

        void
        Move_assign(Move_only_function_base&& Other) noexcept {
            // As specified in N4950 [func.wrap.move.ctor]/22, we are expected to first move the new target,
            // then finally destroy the old target.
            // It is more efficient to do the reverse - this way no temporary storage for the old target will be used.
            // In some cases when some operations are trivial, it can be optimized,
            // as the order change is unobservable, and everything is noexcept here.
            const auto This_impl = Get_impl(Data);
            const auto Other_impl_move = Get_impl(Other.Data)->Move;
            const auto This_impl_destroy = This_impl->Destroy;

            if (!Other_impl_move) {
                // Move is trivial, destroy first if needed
                if (This_impl_destroy) {
                    This_impl_destroy(Data);
                }
                Function_move_large(Data, Other.Data);
            }
            else if (!This_impl_destroy) {
                // Destroy is trivial, just move
                Other_impl_move(Data, Other.Data);
            }
            else {
                // General case involving a temporary
                Move_only_function_data Tmp;

                if (This_impl->Move) {
                    This_impl->Move(Tmp, Data);
                }
                else {
                    Function_move_large(Tmp, Data);
                }

                Other_impl_move(Data, Other.Data);
                This_impl_destroy(Tmp);
            }
            Other.Reset_to_null();
        }

        void
        Swap(Move_only_function_base& Other) noexcept {
            Move_only_function_data Tmp;

            Checked_move(Tmp, Data);
            Checked_move(Data, Other.Data);
            Checked_move(Other.Data, Tmp);
        }

        [[nodiscard]] bool
        Is_null() const noexcept {
            return Data.Impl == nullptr;
        }

        template<class Vt>
        static constexpr bool Large_function_engaged = alignof(Vt) > alignof(max_align_t)
                                                       || sizeof(Vt) > Move_only_function_data::Buf_size<Vt>
                                                       || !std::is_nothrow_move_constructible_v<Vt>;

        [[nodiscard]] auto
        Get_invoke() const noexcept {
            return Get_impl(Data)->Invoke;
        }

        [[nodiscard]] static const Impl_t*
        Get_impl(const Move_only_function_data& Data) noexcept {
            static constexpr Impl_t Null_move_only_function = {
                   Function_not_callable<Rx, Types...>,
                   nullptr,
                   nullptr,
            };

            const auto Ret = static_cast<const Impl_t*>(Data.Impl);
            return Ret ? Ret : &Null_move_only_function;
        }

        template<class Vt, class VtInvQuals>
        [[nodiscard]] static constexpr Impl_t
        Create_impl() noexcept {
            Impl_t Impl{};
            if constexpr (Large_function_engaged<Vt>) {
                Impl.Invoke = Function_inv_large<Vt, VtInvQuals, Rx, Noexcept, Types...>;
                Impl.Move = nullptr;

                if constexpr (std::is_trivially_destructible_v<Vt>) {
                    if constexpr (alignof(Vt) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
                        Impl.Destroy = Function_deallocate_large_default_aligned;
                    }
                    else {
                        Impl.Destroy = Function_deallocate_large_overaligned<alignof(Vt)>;
                    }
                }
                else {
                    Impl.Destroy = Function_destroy_large<Vt>;
                }
            }
            else {
                Impl.Invoke = Function_inv_small<Vt, VtInvQuals, Rx, Noexcept, Types...>;

                if constexpr (std::is_trivially_copyable_v<Vt> && std::is_trivially_destructible_v<Vt>) {
                    if constexpr ((Function_small_copy_size<Vt>) > Minimum_function_size) {
                        Impl.Move = Function_move_memcpy<Function_small_copy_size<Vt>>;
                    }
                    else {
                        Impl.Move = nullptr;
                    }
                }
                else {
                    Impl.Move = Function_move_small<Vt>;
                }

                if constexpr (std::is_trivially_destructible_v<Vt>) {
                    Impl.Destroy = nullptr;
                }
                else {
                    Impl.Destroy = Function_destroy_small<Vt>;
                }
            }
            return Impl;
        }

        template<class Vt, class VtInvQuals>
        [[nodiscard]] static const Impl_t*
        Create_impl_ptr() noexcept {
            static constexpr Impl_t Impl = Create_impl<Vt, VtInvQuals>();
            return &Impl;
        }
    };

    template<class... Signature>
    class Move_only_function_call {
        static_assert((fail<Signature>::value || ...),
                      "std::move_only_function only accepts function types as template arguments, "
                      "with possibly const/ref/noexcept qualifiers.");

        static_assert(sizeof...(Signature) > 0,
                      "Unlike std::function, std::move_only_function does not define class template argument deduction guides.");
    };

    // A script to generate the specializations is at
    // /tools/scripts/move_only_function_specializations.py
    // (Avoiding C++ preprocessor for better IDE navigation and debugging experience)

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...)> : public Move_only_function_base<Rx, false, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = Vt&;

        template<class Vt>
        static constexpr bool Is_callable_from =
               std::is_invocable_r_v<Rx, Vt, Types...> && std::is_invocable_r_v<Rx, Vt&, Types...>;

        Rx
        operator()(Types... Args) {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...)&> : public Move_only_function_base<Rx, false, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = Vt&;

        template<class Vt>
        static constexpr bool Is_callable_from = std::is_invocable_r_v<Rx, Vt&, Types...>;

        Rx
        operator()(Types... Args) & {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) &&> : public Move_only_function_base<Rx, false, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = Vt&&;

        template<class Vt>
        static constexpr bool Is_callable_from = std::is_invocable_r_v<Rx, Vt, Types...>;

        Rx
        operator()(Types... Args) && {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) const> : public Move_only_function_base<Rx, false, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = const Vt&;

        template<class Vt>
        static constexpr bool Is_callable_from =
               std::is_invocable_r_v<Rx, const Vt, Types...> && std::is_invocable_r_v<Rx, const Vt&, Types...>;

        Rx
        operator()(Types... Args) const {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) const&> : public Move_only_function_base<Rx, false, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = const Vt&;

        template<class Vt>
        static constexpr bool Is_callable_from = std::is_invocable_r_v<Rx, const Vt&, Types...>;

        Rx
        operator()(Types... Args) const& {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) const&&> : public Move_only_function_base<Rx, false, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = const Vt&&;

        template<class Vt>
        static constexpr bool Is_callable_from = std::is_invocable_r_v<Rx, const Vt, Types...>;

        Rx
        operator()(Types... Args) const&& {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

#  ifdef __cpp_noexcept_function_type
    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) noexcept> : public Move_only_function_base<Rx, true, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = Vt&;

        template<class Vt>
        static constexpr bool Is_callable_from =
               std::is_nothrow_invocable_r_v<Rx, Vt, Types...> && std::is_nothrow_invocable_r_v<Rx, Vt&, Types...>;

        Rx
        operator()(Types... Args) noexcept {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) & noexcept> : public Move_only_function_base<Rx, true, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = Vt&;

        template<class Vt>
        static constexpr bool Is_callable_from = std::is_nothrow_invocable_r_v<Rx, Vt&, Types...>;

        Rx
        operator()(Types... Args) & noexcept {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) && noexcept> : public Move_only_function_base<Rx, true, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = Vt&&;

        template<class Vt>
        static constexpr bool Is_callable_from = std::is_nothrow_invocable_r_v<Rx, Vt, Types...>;

        Rx
        operator()(Types... Args) && noexcept {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) const noexcept> : public Move_only_function_base<Rx, true, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = const Vt&;

        template<class Vt>
        static constexpr bool Is_callable_from =
               std::is_nothrow_invocable_r_v<Rx, const Vt, Types...> && std::is_nothrow_invocable_r_v<Rx, const Vt&, Types...>;

        Rx
        operator()(Types... Args) const noexcept {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) const & noexcept>
         : public Move_only_function_base<Rx, true, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = const Vt&;

        template<class Vt>
        static constexpr bool Is_callable_from = std::is_nothrow_invocable_r_v<Rx, const Vt&, Types...>;

        Rx
        operator()(Types... Args) const& noexcept {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };

    template<class Rx, class... Types>
    class Move_only_function_call<Rx(Types...) const && noexcept>
         : public Move_only_function_base<Rx, true, Types...> {
    public:
        using result_type = Rx;

        template<class Vt>
        using VtInvQuals = const Vt&&;

        template<class Vt>
        static constexpr bool Is_callable_from = std::is_nothrow_invocable_r_v<Rx, const Vt, Types...>;

        Rx
        operator()(Types... Args) const&& noexcept {
            return this->Get_invoke()(this->Data, std::forward<Types>(Args)...);
        }
    };
#  endif // defined(__cpp_noexcept_function_type)

    template<class... Signature>
    class move_only_function : private Move_only_function_call<Signature...> {
    private:
        using Call = Move_only_function_call<Signature...>;

        // clang-format off
    template <class Fn>
    static constexpr bool Enable_one_arg_constructor =
        !std::is_same_v<std::remove_cvref_t<Fn>, move_only_function>
        && !Is_specialization_v<std::remove_cvref_t<Fn>, std::in_place_type_t>
        && Call::template Is_callable_from<std::decay_t<Fn>>;

    template <class Fn, class... CTypes>
    static constexpr bool Enable_in_place_constructor =
        std::is_constructible_v<std::decay_t<Fn>, CTypes...>
        && Call::template Is_callable_from<std::decay_t<Fn>>;

    template <class Fn, class Ux, class... CTypes>
    static constexpr bool Enable_in_place_list_constructor =
        std::is_constructible_v<std::decay_t<Fn>, std::initializer_list<Ux>&, CTypes...>
        && Call::template Is_callable_from<std::decay_t<Fn>>;
        // clang-format on
    public:
        using typename Call::result_type;

        move_only_function() noexcept {
            this->Construct_with_null();
        }

        move_only_function(nullptr_t) noexcept {
            this->Construct_with_null();
        }

        move_only_function(move_only_function&&) noexcept = default;

        template<class Fn, std::enable_if_t<Enable_one_arg_constructor<Fn>, int> = 0>
        move_only_function(Fn&& Callable) {
            using Vt = std::decay_t<Fn>;
            static_assert(std::is_constructible_v<Vt, Fn>, "Vt should be constructible from Fn. "
                                                           "(N4950 [func.wrap.move.ctor]/6)");

            if constexpr (std::is_member_pointer_v<Vt> || std::is_pointer_v<Vt> || Is_specialization_v<Vt, move_only_function>) {
                if (Callable == nullptr) {
                    this->Construct_with_null();
                    return;
                }
            }

            using VtInvQuals = Call::template VtInvQuals<Vt>;
            this->template Construct_with_fn<Vt, VtInvQuals>(std::forward<Fn>(Callable));
        }

        template<class Fn, class... CTypes, std::enable_if_t<Enable_in_place_constructor<Fn, CTypes...>, int> = 0>
        explicit move_only_function(std::in_place_type_t<Fn>, CTypes&&... Args) {
            using Vt = std::decay_t<Fn>;
            static_assert(std::is_same_v<Vt, Fn>, "Vt should be the same type as Fn. (N4950 [func.wrap.move.ctor]/12)");

            using VtInvQuals = Call::template VtInvQuals<Vt>;
            this->template Construct_with_fn<Vt, VtInvQuals>(std::forward<CTypes>(Args)...);
        }

        template<class Fn, class Ux, class... CTypes, std::enable_if_t<Enable_in_place_list_constructor<Fn, Ux, CTypes...>, int> = 0>
        explicit move_only_function(std::in_place_type_t<Fn>, std::initializer_list<Ux> Li, CTypes&&... Args) {
            using Vt = std::decay_t<Fn>;
            static_assert(std::is_same_v<Vt, Fn>, "Vt should be the same type as Fn. (N4950 [func.wrap.move.ctor]/18)");

            using VtInvQuals = Call::template VtInvQuals<Vt>;
            this->template Construct_with_fn<Vt, VtInvQuals>(Li, std::forward<CTypes>(Args)...);
        }

        ~
        move_only_function() {
            // Do cleanup in this class destructor rather than base,
            // so that if object construction throws, the unnecessary cleanup isn't called.
            this->Checked_destroy(this->Data);
        }

        move_only_function&
        operator=(nullptr_t) noexcept {
            this->Checked_destroy(this->Data);
            this->Reset_to_null();
            return *this;
        }

        move_only_function&
        operator=(move_only_function&& Other)
        // intentionally noexcept(false), leaving the door open to adding allocator support in the future; see GH-2278
        {
            if (this != std::addressof(Other)) {
                this->Move_assign(std::move(Other));
            }
            return *this;
        }

        template<class Fn, std::enable_if_t<std::is_constructible_v<move_only_function, Fn>, int> = 0>
        move_only_function&
        operator=(Fn&& Callable) {
            this->Move_assign(move_only_function{std::forward<Fn>(Callable)});
            return *this;
        }

        [[nodiscard]] explicit
        operator bool() const noexcept {
            return !this->Is_null();
        }

        using Call::operator();

        void
        swap(move_only_function& Other) noexcept {
            this->Swap(Other);
        }

        friend void
        swap(move_only_function& Fn1, move_only_function& Fn2) noexcept {
            Fn1.Swap(Fn2);
        }

        [[nodiscard]] friend bool
        operator==(const move_only_function& This, nullptr_t) noexcept {
            return This.Is_null();
        }
    };

#  ifdef __GNUG__
#    pragma GCC diagnostic pop
#  endif

#else // have std::move_only_function

    using move_only_function = std::move_only_function;

#endif

}

#endif
