#pragma once

#if __cplusplus < 202302L
    #error out of date c++ version, compile with -stdc++=2c
#elif defined(__clang__) && __clang_major__ < 19
    #error out of date clang, compile with latest version
#elif !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 14
    #error out of date g++, compile with latest version
#elif defined(_MSC_VER)
    #error msvc does not yet support the latest c++ features
#else

#include <concepts>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pfs {
    namespace detail {
        template <typename tp_type_t>
        struct is_reference_wrapper : std::false_type {};
        template <typename tp_type_t>
        struct is_reference_wrapper<std::reference_wrapper<tp_type_t>> : std::true_type{};
        
        template <typename tp_type_t, class... tp_arguments_ts>
        concept invocable_or_equality_comparable_with =
            std::invocable<
                tp_type_t,
                tp_arguments_ts...
            > ||
            (... && std::equality_comparable_with<
                tp_type_t,
                tp_arguments_ts
            >);

        struct invoke_or_compare_fn {
            template <
                typename tp_callee_t,
                class... tp_arguments_ts
            >
            auto constexpr operator() [[nodiscard]] (
                tp_callee_t&&        p_callee,
                tp_arguments_ts&&... p_arguments
            )
            const
            noexcept(noexcept(
                std::invoke(
                    std::declval<tp_callee_t>(),
                    std::declval<tp_arguments_ts>()...
                )
            ))
            -> decltype(
                std::invoke(
                    std::forward<tp_callee_t>(p_callee),
                    std::forward<tp_arguments_ts>(p_arguments)...
                )
            ) {
                return std::invoke(
                    std::forward<tp_callee_t>(p_callee),
                    std::forward<tp_arguments_ts>(p_arguments)...
                );
            }
            template <
                typename tp_comparable_t,
                class... tp_arguments_ts
            >
            requires (
                !std::invocable<
                    tp_comparable_t,
                    tp_arguments_ts...
                >
            )
            auto constexpr operator() [[nodiscard]] (
                tp_comparable_t&&    p_comparable,
                tp_arguments_ts&&... p_arguments
            )
            const
            noexcept(noexcept(
                (... && (std::declval<tp_comparable_t>() == std::declval<tp_arguments_ts>()))
            ))
            -> decltype((... && (p_comparable == p_arguments))) {
                return (... && (p_comparable == p_arguments));
            }
        };
        auto constexpr invoke_or_compare = invoke_or_compare_fn{};
        
        template <
            typename tp_callee_t,
            class... tp_arguments_ts
        >
        using invoke_or_compare_result_t = decltype(invoke_or_compare(
            std::declval<tp_callee_t>(),
            std::declval<tp_arguments_ts>()...
        ));

        template <typename tp_type_t>
        struct combinator_traits;

        template <std::size_t tp_index>
        struct get_impl_fn;

        template <std::size_t tp_index>
        struct get_possibly_reference_wrapped_impl_fn;
        
        template <std::size_t tp_index, class... tp_types_ts>
        struct storage_impl : storage_impl<tp_index + 1, tp_types_ts...> {
            [[no_unique_address]] tp_types_ts...[sizeof...(tp_types_ts) - 1 - tp_index] m_value;
        };
        template <class... tp_types_ts>
        struct storage_impl<sizeof...(tp_types_ts) - 1, tp_types_ts...> {
            [[no_unique_address]] tp_types_ts...[0] m_value;
        };
        template <std::size_t tp_index>
        struct storage_impl<tp_index> {};
        
        template <class... tp_types_ts>
        storage_impl(tp_types_ts&&...) -> storage_impl<0, tp_types_ts...>;

        template <class... tp_types_ts>
        struct storage : storage_impl<0, tp_types_ts...> {
        protected:
            auto constexpr static m_size = sizeof...(tp_types_ts);
            auto constexpr static m_is_owning = !(... && is_reference_wrapper<std::remove_cvref_t<tp_types_ts>>::value);
        private:
            template <std::size_t tp_index>
            using m_index_storage_t = storage_impl<
                m_size - 1 - tp_index,
                tp_types_ts...
            >;
        protected:
            template <std::size_t tp_index>
            using m_possibly_reference_wrapped_element_t = decltype(m_index_storage_t<tp_index>::m_value);
        private:
            template <typename tp_type_t>
            struct m_element_type : std::type_identity<tp_type_t> {};
            template <typename tp_type_t>
            struct m_element_type<std::reference_wrapper<tp_type_t>> : std::type_identity<typename tp_type_t::type> {};
        protected:
            template <std::size_t tp_index>
            using m_element_t = typename m_element_type<m_possibly_reference_wrapped_element_t<tp_index>>::type;

            template <
                std::size_t tp_index,
                typename    tp_self_t
            >
            auto constexpr get_possibly_reference_wrapped(this tp_self_t&& p_self)
            noexcept(noexcept(std::declval<tp_self_t>().storage::template m_index_storage_t<tp_index>::m_value))
            -> decltype((std::forward<tp_self_t>(p_self).storage::template m_index_storage_t<tp_index>::m_value)) {
                return std::forward<tp_self_t>(p_self).storage::template m_index_storage_t<tp_index>::m_value;
            }

            template <
                std::size_t tp_index,
                typename    tp_self_t
            >
            requires (is_reference_wrapper<m_possibly_reference_wrapped_element_t<tp_index>>::value)
            auto constexpr get(this tp_self_t&& p_self)
            noexcept(noexcept(std::forward_like<tp_self_t>(std::declval<tp_self_t>().storage::template get_possibly_reference_wrapped<tp_index>().get())))
            -> decltype((std::forward_like<tp_self_t>(std::forward<tp_self_t>(p_self).storage::template get_possibly_reference_wrapped<tp_index>().get()))) {
                return std::forward_like<tp_self_t>(std::forward<tp_self_t>(p_self).storage::template get_possibly_reference_wrapped<tp_index>().get());
            }
            template <
                std::size_t tp_index,
                typename    tp_self_t
            >
            auto constexpr get(this tp_self_t&& p_self)
            noexcept(noexcept(std::forward_like<tp_self_t>(std::declval<tp_self_t>().storage::template get_possibly_reference_wrapped<tp_index>())))
            -> decltype((std::forward_like<tp_self_t>(std::forward<tp_self_t>(p_self).storage::template get_possibly_reference_wrapped<tp_index>()))) {
                return std::forward_like<tp_self_t>(std::forward<tp_self_t>(p_self).storage::template get_possibly_reference_wrapped<tp_index>());
            }
        };

        template <typename tp_derived_t>
        struct combinator_base;
        template <
            template <class...> class tp_derived_template_tp,
            class...                  tp_elements_outer_ts
        >
        struct combinator_base<tp_derived_template_tp<tp_elements_outer_ts...>>
        : storage<std::remove_cvref_t<tp_elements_outer_ts>...> {
            template <typename tp_type_t>
            friend struct combinator_traits;

            template <std::size_t tp_index>
            friend struct get_impl_fn;

            template <std::size_t tp_index>
            friend struct get_possibly_reference_wrapped_impl_fn;

        private:
            using m_base_t = storage<std::remove_cvref_t<tp_elements_outer_ts>...>;
        protected:
            using m_base_t::m_size;
            using m_base_t::m_is_owning;

            using m_base_t::get_possibly_reference_wrapped;
            using m_base_t::get;
            using m_base_t::m_element_t;
            using m_base_t::m_possibly_reference_wrapped_element_t;

            template <class... tp_elements_inner_ts>
            using m_template_tp = tp_derived_template_tp<tp_elements_inner_ts...>;
        };
        
        template <typename tp_derived_t>
        struct adaptor_base {};

        template <typename tp_derived_t>
        struct modifier_closure_base {};

        template <typename tp_type_t>
        struct combinator_traits {
            using m_without_cvref_t = std::remove_cvref_t<tp_type_t>;
            auto constexpr static m_size      = m_without_cvref_t::m_size;
            auto constexpr static m_is_owning = m_without_cvref_t::m_is_owning;
            template <std::size_t tp_index>
            using m_possibly_reference_wrapped_element_t = typename m_without_cvref_t::template m_possibly_reference_wrapped_element_t<tp_index>;
            template <std::size_t tp_index>
            using m_element_t = typename m_without_cvref_t::template m_element_t<tp_index>;
            template <class... tp_elements_inner_ts>
            using m_template_tp = m_without_cvref_t::template m_template_tp<tp_elements_inner_ts...>;
        };

        template <std::size_t tp_index>
        struct get_impl_fn {
            template <typename tp_combinator_t>
            auto constexpr operator()(tp_combinator_t&& p_combinator)
            const
            noexcept(noexcept(std::forward<tp_combinator_t>(p_combinator).template get<tp_index>()))
            -> decltype(std::forward<tp_combinator_t>(p_combinator).template get<tp_index>()) {
                return std::forward<tp_combinator_t>(p_combinator).template get<tp_index>();
            }
        };
        template <std::size_t tp_index>
        auto constexpr get_impl = get_impl_fn<tp_index>{};

        template <std::size_t tp_index>
        struct get_possibly_reference_wrapped_impl_fn {
            template <typename tp_combinator_t>
            auto constexpr operator()(tp_combinator_t&& p_combinator)
            const
            noexcept(noexcept(std::forward<tp_combinator_t>(p_combinator).template get_possibly_reference_wrapped<tp_index>()))
            -> decltype(std::forward<tp_combinator_t>(p_combinator).template get_possibly_reference_wrapped<tp_index>()) {
                return std::forward<tp_combinator_t>(p_combinator).template get_possibly_reference_wrapped<tp_index>();
            }
        };
        template <std::size_t tp_index>
        auto constexpr get_possibly_reference_wrapped_impl = get_possibly_reference_wrapped_impl_fn<tp_index>{};

        template <typename tp_type_t>
        concept combinator = 
            std::derived_from<
                std::remove_cvref_t<tp_type_t>,
                combinator_base<std::remove_cvref_t<tp_type_t>>
            >;

        template <typename tp_type_t>
        concept adaptor = 
            std::derived_from<
                std::remove_cvref_t<tp_type_t>,
                adaptor_base<std::remove_cvref_t<tp_type_t>>
            >;

        template <typename tp_type_t>
        concept modifier_closure = 
            std::derived_from<
                std::remove_cvref_t<tp_type_t>,
                modifier_closure_base<std::remove_cvref_t<tp_type_t>>
            >;

        template <typename tp_combinator_t>
        concept owning_combinator =
            combinator<tp_combinator_t> &&
            combinator_traits<tp_combinator_t>::m_is_owning; //a combinator owns all of it's elements, that is, none of it's elements are of type std::reference_wrapper<T>

        template <typename tp_type_t>
        concept combinator_or_modifier_closure =
            combinator<tp_type_t> ||
            modifier_closure<tp_type_t>;

        inline namespace adl_exposed { // note: handy for disambiguation, and also clarity
            template <std::size_t tp_index, combinator_or_modifier_closure tp_type_t>
            auto constexpr get(tp_type_t&& p_combinator)
            noexcept(noexcept(get_impl<tp_index>(std::declval<tp_type_t>())))
            -> decltype(get_impl<tp_index>(std::forward<tp_type_t>(p_combinator))) {
                return get_impl<tp_index>(std::forward<tp_type_t>(p_combinator));
            }
        }

        template <
            typename tp_modifier_t,
            class... tp_types_ts
        >
        struct partial_modifier : storage<std::remove_cvref_t<tp_types_ts>...>, modifier_closure_base<
            partial_modifier<
                tp_modifier_t,
                tp_types_ts...
            >
        >{
        private:
            template <
                combinator     tp_combinator_t,
                typename       tp_self_t,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&  p_self,
                tp_combinator_t&& p_combinator,
                std::index_sequence<tp_is...>
            )
            noexcept(noexcept(
                std::invoke(
                    std::declval<tp_modifier_t>(),
                    std::declval<tp_self_t>().template get_possibly_reference_wrapped<tp_is>()...
                )
            ))
            -> decltype (
                std::invoke(
                    tp_modifier_t{},
                    std::forward<tp_self_t>(p_self).template get_possibly_reference_wrapped<tp_is>()...
                )
            ) {
                return std::invoke(
                    tp_modifier_t{},
                    std::forward<tp_self_t>(p_self).template get_possibly_reference_wrapped<tp_is>()...
                );
            }
        public:
            template <
                combinator tp_combinator_t,
                typename   tp_self_t
            >
            auto constexpr operator()[[nodiscard]](
                this tp_self_t&&  p_self,
                tp_combinator_t&& p_combinator
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::declval<tp_combinator_t>(),
                    std::make_index_sequence<sizeof...(tp_types_ts)>{}
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::forward<tp_combinator_t>(p_combinator),
                    std::make_index_sequence<sizeof...(tp_types_ts)>{}
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::forward<tp_combinator_t>(p_combinator),
                    std::make_index_sequence<sizeof...(tp_types_ts)>{}
                );
            }
        };

        template <
            modifier_closure tp_left_modifier_closure,
            modifier_closure tp_right_modifier_closure
        >
        struct pipe_modifier : modifier_closure_base<pipe_modifier<
            tp_left_modifier_closure,
            tp_right_modifier_closure
        >> {
            [[no_unique_address]] tp_left_modifier_closure  m_left_modifier_closure;
            [[no_unique_address]] tp_right_modifier_closure m_right_modifier_closure;

            template <
                combinator tp_combinator_t,
                typename   tp_self_t
            >
            auto constexpr operator()[[nodiscard]](
                this tp_self_t&&  p_self,
                tp_combinator_t&& p_combinator
            )
            noexcept(noexcept(
                std::invoke(
                    std::declval<tp_self_t>().m_left_modifier_closure,
                    std::invoke(
                        std::declval<tp_self_t>().m_right_modifier_closure,
                        std::declval<tp_combinator_t>()
                    )
                )
            ))
            -> decltype(
                std::invoke(
                    std::forward<tp_self_t>(p_self).m_left_modifier_closure,
                    std::invoke(
                        std::forward<tp_self_t>(p_self).m_right_modifier_closure,
                        std::forward<tp_combinator_t>(p_combinator)
                    )
                )
            ) {
                return std::invoke(
                    std::forward<tp_self_t>(p_self).m_left_modifier_closure,
                    std::invoke(
                        std::forward<tp_self_t>(p_self).m_right_modifier_closure,
                        std::forward<tp_combinator_t>(p_combinator)
                    )
                );
            }
        };

        template <typename tp_derived_t>
        struct modifier_base {
            template <class... tp_types_ts>
            auto constexpr operator()[[nodiscard]](tp_types_ts&&... p_arguments)
            const noexcept(noexcept(
                partial_modifier<
                    tp_derived_t,
                    tp_types_ts...
                >{std::declval<tp_types_ts>()...}
            ))
            -> decltype(
                partial_modifier<
                    tp_derived_t,
                    tp_types_ts...
                >{std::forward<tp_types_ts>(p_arguments)...}
            ) {
                return partial_modifier<
                    tp_derived_t,
                    tp_types_ts...
                >{std::forward<tp_types_ts>(p_arguments)...};
            }
        };

        template <typename tp_type_t>
        concept modifier = 
            std::derived_from<
                std::remove_cvref_t<tp_type_t>,
                modifier_base<std::remove_cvref_t<tp_type_t>>
            >;
    }

    namespace detail {
        struct combinator_append_element_fn {
            template <
                pfs::detail::combinator tp_combinator_t,
                typename                tp_element_t,
                std::size_t...          tp_is
            >
            auto constexpr operator() [[nodiscard]] (
                std::index_sequence<tp_is...>,
                tp_combinator_t&& p_combinator,
                tp_element_t&&    p_element
            )
            const
            noexcept(noexcept(
                typename combinator_traits<tp_combinator_t>::template m_template_tp<
                    typename combinator_traits<tp_combinator_t>::template m_possibly_reference_wrapped_element_t<tp_is>...,
                    std::remove_cvref_t<tp_element_t>
                >{
                    get_possibly_reference_wrapped_impl<tp_is>(std::declval<tp_combinator_t>())...,
                    std::declval<tp_element_t>()
                }
            ))
            -> decltype(
                typename combinator_traits<tp_combinator_t>::template m_template_tp<
                    typename combinator_traits<tp_combinator_t>::template m_possibly_reference_wrapped_element_t<tp_is>...,
                    std::remove_cvref_t<tp_element_t>
                >{
                    get_possibly_reference_wrapped_impl<tp_is>(std::forward<tp_combinator_t>(p_combinator))...,
                    std::forward<tp_element_t>(p_element)
                }
            ) {
                return typename combinator_traits<tp_combinator_t>::template m_template_tp<
                    typename combinator_traits<tp_combinator_t>::template m_possibly_reference_wrapped_element_t<tp_is>...,
                    std::remove_cvref_t<tp_element_t>
                >{
                    get_possibly_reference_wrapped_impl<tp_is>(std::forward<tp_combinator_t>(p_combinator))...,
                    std::forward<tp_element_t>(p_element)
                };
            }
        };
        auto constexpr combinator_append_element = combinator_append_element_fn{};

        struct merge_combinators_fn {
            template <
                pfs::detail::combinator tp_combinator1_t,
                pfs::detail::combinator tp_combinator2_t,
                std::size_t...          tp_is1,
                std::size_t...          tp_is2
            >
            auto constexpr operator() [[nodiscard]] (
                std::index_sequence<tp_is1...>,
                std::index_sequence<tp_is2...>,
                tp_combinator1_t&& p_combinator2,
                tp_combinator2_t&& p_combinator1
            )
            const
            noexcept(noexcept( //could just rely on ctad, specifying argumenst just for clangd crash workaround
                typename combinator_traits<tp_combinator1_t>::template m_template_tp<
                    typename combinator_traits<tp_combinator1_t>::template m_possibly_reference_wrapped_element_t<tp_is1>...,
                    typename combinator_traits<tp_combinator2_t>::template m_possibly_reference_wrapped_element_t<tp_is2>...
                >{
                    get_possibly_reference_wrapped_impl<tp_is1>(std::declval<tp_combinator1_t>())...,
                    get_possibly_reference_wrapped_impl<tp_is2>(std::declval<tp_combinator2_t>())...
                }
            ))
            -> decltype( //could just rely on ctad, specifying argumenst just for clangd crash workaround
                typename combinator_traits<tp_combinator1_t>::template m_template_tp<
                    typename combinator_traits<tp_combinator1_t>::template m_possibly_reference_wrapped_element_t<tp_is1>...,
                    typename combinator_traits<tp_combinator2_t>::template m_possibly_reference_wrapped_element_t<tp_is2>...
                >{
                    get_possibly_reference_wrapped_impl<tp_is1>(std::forward<tp_combinator1_t>(p_combinator1))...,
                    get_possibly_reference_wrapped_impl<tp_is2>(std::forward<tp_combinator2_t>(p_combinator2))...
                }
            ) { //could just rely on ctad, specifying argumenst just for clangd crash workaround
                return typename combinator_traits<tp_combinator1_t>::template m_template_tp<
                    typename combinator_traits<tp_combinator1_t>::template m_possibly_reference_wrapped_element_t<tp_is1>...,
                    typename combinator_traits<tp_combinator2_t>::template m_possibly_reference_wrapped_element_t<tp_is2>...
                >{
                    get_possibly_reference_wrapped_impl<tp_is1>(std::forward<tp_combinator1_t>(p_combinator1))...,
                    get_possibly_reference_wrapped_impl<tp_is2>(std::forward<tp_combinator2_t>(p_combinator2))...
                };
            }
        };
        auto constexpr merge_combinators = merge_combinators_fn{};

        template <typename tp_like_t>
        struct forward_like_if_invocable_fn {
            template <
                typename tp_callee_t,
                class... tp_arguments_ts
            >
            requires (
                std::invocable<
                    tp_callee_t,
                    tp_arguments_ts...
                >
            )
            auto constexpr operator() [[nodiscard]] (
                tp_callee_t&&        p_callee,
                tp_arguments_ts&&... p_arguments
            )
            const noexcept
            -> decltype(std::forward_like<tp_like_t>(p_callee)) {
                return std::forward_like<tp_like_t>(p_callee);
            }
            template <
                typename tp_callee_t,
                class... tp_arguments_ts
            >
            requires (
                !std::invocable<
                    tp_callee_t,
                    tp_arguments_ts...
                >
            )
            auto constexpr operator() [[nodiscard]] (
                tp_callee_t&&        p_callee,
                tp_arguments_ts&&... p_arguments
            )
            const noexcept
            -> decltype(std::forward<tp_callee_t>(p_callee)) {
                return std::forward<tp_callee_t>(p_callee);
            }
        };
        template <typename tp_like_t>
        auto constexpr forward_like_if_invocable = forward_like_if_invocable_fn<tp_like_t>{};
    }

    namespace detail {
        template <template <class...> class tp_combinator_tp>
        struct adaptor_for : adaptor_base<adaptor_for<tp_combinator_tp>> {
            template <class... tp_elements_ts>
            requires (
                std::cmp_not_equal(
                    sizeof...(tp_elements_ts),
                    0
                )
            )
            auto constexpr operator()[[nodiscard]](tp_elements_ts&&... p_elements)
            const noexcept(noexcept(
                tp_combinator_tp<std::remove_cvref_t<tp_elements_ts>...>{
                    std::declval<tp_elements_ts>()...
                }
            ))
            -> decltype(
                tp_combinator_tp<std::remove_cvref_t<tp_elements_ts>...>{
                    std::forward<tp_elements_ts>(p_elements)...
                }
            ) {
                return tp_combinator_tp<std::remove_cvref_t<tp_elements_ts>...>{
                    std::forward<tp_elements_ts>(p_elements)...
                };
            }
        };

        template <class... tp_elements_outer_ts>
        requires (
            std::cmp_equal(
                sizeof...(tp_elements_outer_ts),
                1
            )
        )
        struct identity_combinator : combinator_base<identity_combinator<tp_elements_outer_ts...>> {
            using m_base_t = combinator_base<identity_combinator<tp_elements_outer_ts...>>;

            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<0>(std::declval<tp_self_t>())),
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<0>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<0>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr identity = detail::adaptor_for<detail::identity_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct compose_combinator : combinator_base<compose_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<compose_combinator<tp_elements_ts...>>;

            template <std::size_t tp_index>
            struct impl_wrap { // note: prevents endless recursion
                template <
                    typename    tp_self_t,
                    class...    tp_arguments_ts
                >
                auto constexpr static impl(
                    tp_self_t&&          p_self,
                    tp_arguments_ts&&... p_arguments
                )
                noexcept(noexcept(
                    std::invoke(
                        std::forward_like<tp_self_t>(
                            adl_exposed::get<tp_index>(
                                std::declval<tp_self_t>()
                            )
                        ),
                        impl_wrap<tp_index - 1>::impl(
                            std::declval<tp_self_t>(),
                            std::declval<tp_arguments_ts>()...
                        )
                    )
                ))
                -> decltype(
                    std::invoke(
                        std::forward_like<tp_self_t>(
                            adl_exposed::get<tp_index>(
                                std::forward<tp_self_t>(
                                    p_self
                                )
                            )
                        ),
                        impl_wrap<tp_index - 1>::impl(
                            std::forward<tp_self_t>(p_self),
                            std::forward<tp_arguments_ts>(p_arguments)...
                        )
                    )
                ) {
                    return std::invoke(
                        std::forward_like<tp_self_t>(
                            adl_exposed::get<tp_index>(
                                std::forward<tp_self_t>(
                                    p_self
                                )
                            )
                        ),
                        impl_wrap<tp_index - 1>::impl(
                            std::forward<tp_self_t>(p_self),
                            std::forward<tp_arguments_ts>(p_arguments)...
                        )
                    );
                }
            };

            template <std::size_t tp_index>
            requires (std::cmp_equal(
                tp_index,
                0
            ))
            struct impl_wrap<tp_index> {
                template <
                    typename tp_self_t,
                    class... tp_arguments_ts
                >
                auto constexpr static impl(
                    tp_self_t&&          p_self,
                    tp_arguments_ts&&... p_arguments
                )
                noexcept(noexcept(
                    std::invoke(
                        std::forward_like<tp_self_t>(adl_exposed::get<0>(std::declval<tp_self_t>())),
                        std::declval<tp_arguments_ts>()...
                    )
                ))
                -> decltype(
                    std::invoke(
                        std::forward_like<tp_self_t>(adl_exposed::get<0>(std::forward<tp_self_t>(p_self))),
                        std::forward<tp_arguments_ts>(p_arguments)...
                    )
                ) {
                    return std::invoke(
                        std::forward_like<tp_self_t>(adl_exposed::get<0>(std::forward<tp_self_t>(p_self))),
                        std::forward<tp_arguments_ts>(p_arguments)...
                    );
                }
            };

        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                impl_wrap<m_base_t::m_size - 1>::impl(
                    std::declval<tp_self_t>(),
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                impl_wrap<m_base_t::m_size - 1>::impl(
                    std::forward<tp_self_t>(p_self),
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return impl_wrap<m_base_t::m_size - 1>::impl(
                    std::forward<tp_self_t>(p_self),
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr compose = detail::adaptor_for<detail::compose_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct fork_combinator : combinator_base<fork_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<fork_combinator<tp_elements_ts...>>;

            template <
                typename       tp_arguments_t,
                std::size_t    tp_index = 0,
                std::size_t... tp_indices
            >
            struct m_viable_indices_impl;
            template <
                typename       tp_arguments_t,
                std::size_t... tp_indices
            >
            struct m_viable_indices_impl<
                tp_arguments_t,
                combinator_traits<fork_combinator>::m_size,
                tp_indices...
            > : std::type_identity<std::index_sequence<tp_indices...>> {};
            template <
                class...       tp_arguments_ts,
                std::size_t    tp_index,
                std::size_t... tp_indices
            >
            requires (std::cmp_not_equal(
                tp_index,
                combinator_traits<fork_combinator>::m_size
            ))
            struct m_viable_indices_impl<
                std::tuple<tp_arguments_ts...>,
                tp_index,
                tp_indices...
            > : std::conditional_t<
                invocable_or_equality_comparable_with<
                    typename m_base_t::template m_possibly_reference_wrapped_element_t<tp_index>,
                    tp_arguments_ts...
                >,
                m_viable_indices_impl<
                    std::tuple<tp_arguments_ts...>,
                    tp_index + 1,
                    tp_indices...,
                    tp_index
                >,
                m_viable_indices_impl<
                    std::tuple<tp_arguments_ts...>,
                    tp_index + 1,
                    tp_indices...
                >
            >{};
            template <class... tp_arguments_ts>
            auto constexpr static m_viable_indices = typename m_viable_indices_impl<std::tuple<tp_arguments_ts...>>::type{};

            template <
                typename       tp_self_t,
                class...       tp_arguments_ts,
                std::size_t... tp_is
            >
            auto constexpr invoke_viable(
                this tp_self_t&&     p_self,
                std::index_sequence<tp_is...>,
                tp_arguments_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::tuple<
                    invoke_or_compare_result_t<
                        typename m_base_t::template m_possibly_reference_wrapped_element_t<tp_is>,
                        tp_arguments_ts...
                    >...
                >{
                    invoke_or_compare(
                        forward_like_if_invocable<tp_self_t>(get_possibly_reference_wrapped_impl<tp_is>(std::declval<tp_self_t>())),
                        std::declval<tp_arguments_ts>()...
                    )...
                }
            ))
            -> decltype(
                std::tuple<
                    invoke_or_compare_result_t<
                        typename m_base_t::template m_possibly_reference_wrapped_element_t<tp_is>,
                        tp_arguments_ts...
                    >...
                >{
                    invoke_or_compare(
                        forward_like_if_invocable<tp_self_t>(get_possibly_reference_wrapped_impl<tp_is>(std::forward<tp_self_t>(p_self))),
                        p_arguments...
                    )...
                }
            ) {
                return std::tuple<
                    invoke_or_compare_result_t<
                        typename m_base_t::template m_possibly_reference_wrapped_element_t<tp_is>,
                        tp_arguments_ts...
                    >...
                >{
                    invoke_or_compare(
                        forward_like_if_invocable<tp_self_t>(get_possibly_reference_wrapped_impl<tp_is>(std::forward<tp_self_t>(p_self))),
                        p_arguments...
                    )...
                };
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            ) { //shoud be the below, however it breaks clangd
            //noexcept(noexcept(
            //    std::declval<tp_self_t>().invoke_viable(
            //        m_viable_indices<tp_parameter_ts...>,
            //        std::declval<tp_parameter_ts>()...
            //    )
            //))
            //-> decltype(
            //    std::forward<tp_self_t>(p_self).invoke_viable(
            //        m_viable_indices<tp_parameter_ts...>,
            //        std::forward<tp_parameter_ts>(p_arguments)...
            //    )
            //) {
                return std::forward<tp_self_t>(p_self).invoke_viable(
                    m_viable_indices<tp_parameter_ts...>,
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr fork = detail::adaptor_for<detail::fork_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct all_combinator : combinator_base<all_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<all_combinator<tp_elements_ts...>>;

            template <
                typename       tp_self_t,
                class...       tp_argument_ts,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_ts&&... p_arguments
            )
            noexcept(noexcept(
                (... && invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::declval<tp_self_t>())),
                    std::declval<tp_argument_ts>()...
                ))
            ))
            -> decltype(
                (... && invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)...
                ))
            ) {
                return (... && invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)...
                ));
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr all = detail::adaptor_for<detail::all_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct piecewise_all_combinator : combinator_base<piecewise_all_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<piecewise_all_combinator<tp_elements_ts...>>;

            template <
                typename       tp_self_t,
                class...       tp_argument_ts,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_ts&&... p_arguments
            )
            noexcept(noexcept(
                (... && invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::declval<tp_self_t>())),
                    std::declval<tp_argument_ts>()
                ))
            ))
            -> decltype(
                (... && invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)
                ))
            ) {
                return (... && invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)
                ));
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr piecewise_all = detail::adaptor_for<detail::piecewise_all_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct cartesian_all_combinator : combinator_base<cartesian_all_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<cartesian_all_combinator<tp_elements_ts...>>;

            template <
                typename       tp_self_t,
                typename       tp_argument_t,
                std::size_t... tp_is
            >
            auto constexpr invoke_each_element_with(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_t&&     p_argument
            )
            noexcept(noexcept(
                (... && invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::declval<tp_self_t>())),
                    std::declval<tp_argument_t>()
                ))
            ))
            -> decltype(
                (... && invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_t>(p_argument)
                ))
            ) {
                return (... && invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_t>(p_argument)
                ));
            }
            template <
                typename       tp_self_t,
                class...       tp_argument_ts,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_ts&&... p_arguments
            )
            noexcept(noexcept(
                (... && std::forward<tp_self_t>(p_self).invoke_each_element_with(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_argument_ts>()
                ))
            ))
            -> decltype(
                (... && std::forward<tp_self_t>(p_self).invoke_each_element_with(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_argument_ts>(p_arguments)
                ))
            ) {
                return (... && std::forward<tp_self_t>(p_self).invoke_each_element_with(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_argument_ts>(p_arguments)
                ));
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr cartesian_all = detail::adaptor_for<detail::cartesian_all_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct any_combinator : combinator_base<any_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<any_combinator<tp_elements_ts...>>;

            template <
                typename       tp_self_t,
                class...       tp_argument_ts,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_ts&&... p_arguments
            )
            noexcept(noexcept(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::declval<tp_self_t>())),
                    std::declval<tp_argument_ts>()...
                ))
            ))
            -> decltype(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)...
                ))
            ) {
                return (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)...
                ));
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr any = detail::adaptor_for<detail::any_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct piecewise_any_combinator : combinator_base<piecewise_any_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<piecewise_any_combinator<tp_elements_ts...>>;

            template <
                typename       tp_self_t,
                class...       tp_argument_ts,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_ts&&... p_arguments
            )
            noexcept(noexcept(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::declval<tp_self_t>())),
                    std::declval<tp_argument_ts>()
                ))
            ))
            -> decltype(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)
                ))
            ) {
                return (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)
                ));
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr piecewise_any = detail::adaptor_for<detail::piecewise_any_combinator>{};
    
    namespace detail {
        template <class... tp_elements_ts>
        struct cartesian_any_combinator : combinator_base<cartesian_any_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<cartesian_any_combinator<tp_elements_ts...>>;

            template <
                typename       tp_self_t,
                typename       tp_argument_t,
                std::size_t... tp_is
            >
            auto constexpr invoke_each_element_with(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_t&&     p_argument
            )
            noexcept(noexcept(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::declval<tp_self_t>())),
                    std::declval<tp_argument_t>()
                ))
            ))
            -> decltype(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_t>(p_argument)
                ))
            ) {
                return (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_t>(p_argument)
                ));
            }
            template <
                typename       tp_self_t,
                class...       tp_argument_ts,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_ts&&... p_arguments
            )
            noexcept(noexcept(
                (... || std::forward<tp_self_t>(p_self).invoke_each_element_with(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_argument_ts>()
                ))
            ))
            -> decltype(
                (... || std::forward<tp_self_t>(p_self).invoke_each_element_with(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_argument_ts>(p_arguments)
                ))
            ) {
                return (... || std::forward<tp_self_t>(p_self).invoke_each_element_with(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_argument_ts>(p_arguments)
                ));
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr cartesian_any = detail::adaptor_for<detail::cartesian_any_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct none_combinator : combinator_base<none_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<none_combinator<tp_elements_ts...>>;

            template <
                typename       tp_self_t,
                class...       tp_argument_ts,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_ts&&... p_arguments
            )
            noexcept(noexcept(
                !(... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::declval<tp_self_t>())),
                    std::declval<tp_argument_ts>()...
                ))
            ))
            -> decltype(
                !(... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)...
                ))
            ) {
                return !(... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)...
                ));
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr none = detail::adaptor_for<detail::none_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct piecewise_none_combinator : combinator_base<piecewise_none_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<piecewise_none_combinator<tp_elements_ts...>>;

            template <
                typename       tp_self_t,
                class...       tp_argument_ts,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_ts&&... p_arguments
            )
            noexcept(noexcept(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::declval<tp_self_t>())),
                    std::declval<tp_argument_ts>()
                ))
            ))
            -> decltype(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)
                ))
            ) {
                return (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_ts>(p_arguments)
                ));
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr piecewise_none = detail::adaptor_for<detail::piecewise_none_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct cartesian_none_combinator : combinator_base<cartesian_none_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<cartesian_none_combinator<tp_elements_ts...>>;

            template <
                typename       tp_self_t,
                typename       tp_argument_t,
                std::size_t... tp_is
            >
            auto constexpr invoke_each_element_with(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_t&&     p_argument
            )
            noexcept(noexcept(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::declval<tp_self_t>())),
                    std::declval<tp_argument_t>()
                ))
            ))
            -> decltype(
                (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_t>(p_argument)
                ))
            ) {
                return (... || invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<tp_is>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_argument_t>(p_argument)
                ));
            }
            template <
                typename       tp_self_t,
                class...       tp_argument_ts,
                std::size_t... tp_is
            >
            auto constexpr impl(
                this tp_self_t&&    p_self,
                std::index_sequence<tp_is...>,
                tp_argument_ts&&... p_arguments
            )
            noexcept(noexcept(
                !(... || std::forward<tp_self_t>(p_self).invoke_each_element_with(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_argument_ts>()
                ))
            ))
            -> decltype(
                !(... || std::forward<tp_self_t>(p_self).invoke_each_element_with(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_argument_ts>(p_arguments)
                ))
            ) {
                return !(... || std::forward<tp_self_t>(p_self).invoke_each_element_with(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_argument_ts>(p_arguments)
                ));
            }
        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                std::declval<tp_self_t>().impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return std::forward<tp_self_t>(p_self).impl(
                    std::make_index_sequence<m_base_t::m_size>{},
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr cartesian_none = detail::adaptor_for<detail::cartesian_none_combinator>{};

    namespace detail {
        template <class... tp_elements_ts>
        struct negate_combinator : combinator_base<negate_combinator<tp_elements_ts...>> {
        private:
            using m_base_t = combinator_base<negate_combinator<tp_elements_ts...>>;

        public:
            template <
                typename tp_self_t,
                class... tp_parameter_ts
            >
            auto constexpr operator()(
                this tp_self_t&&     p_self,
                tp_parameter_ts&&... p_arguments
            )
            noexcept(noexcept(
                !invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<0>(std::declval<tp_self_t>())),
                    std::declval<tp_parameter_ts>()...
                )
            ))
            -> decltype(
                invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<0>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_parameter_ts>(p_arguments)...
                )
            ) {
                return !invoke_or_compare(
                    forward_like_if_invocable<tp_self_t>(adl_exposed::get<0>(std::forward<tp_self_t>(p_self))),
                    std::forward<tp_parameter_ts>(p_arguments)...
                );
            }
        };
    }
    auto constexpr negate = detail::adaptor_for<detail::negate_combinator>{};

    namespace detail {
        struct bind_modifier : modifier_base<bind_modifier> {
            using modifier_base<bind_modifier>::operator();
            template <
                combinator tp_combinator_t,
                class...   tp_arguments_ts
            >
            auto constexpr operator()[[nodiscard]](
                tp_combinator_t&&    p_combinator,
                tp_arguments_ts&&... p_arguments
            )
            const noexcept(noexcept(
                identity(std::bind(
                    std::declval<tp_combinator_t>(),
                    std::declval<tp_arguments_ts>()...
                ))
            ))
            -> decltype(
                identity(std::bind(
                    std::forward<tp_combinator_t>(p_combinator),
                    std::forward<tp_arguments_ts>(p_arguments)...
                ))
            ) {
                return identity(std::bind(
                    std::forward<tp_combinator_t>(p_combinator),
                    std::forward<tp_arguments_ts>(p_arguments)...
                ));
            }
        };
    }
    auto constexpr bind = detail::bind_modifier{};

    namespace detail {
        struct bind_front_modifier : modifier_base<bind_front_modifier> {
            using modifier_base<bind_front_modifier>::operator();
            template <
                combinator tp_combinator_t,
                class...   tp_arguments_ts
            >
            auto constexpr operator()[[nodiscard]](
                tp_combinator_t&&    p_combinator,
                tp_arguments_ts&&... p_arguments
            )
            const noexcept(noexcept(
                identity(std::bind_front(
                    std::declval<tp_combinator_t>(),
                    std::declval<tp_arguments_ts>()...
                ))
            ))
            -> decltype(
                identity(std::bind_front(
                    std::forward<tp_combinator_t>(p_combinator),
                    std::forward<tp_arguments_ts>(p_arguments)...
                ))
            ) {
                return identity(std::bind_front(
                    std::forward<tp_combinator_t>(p_combinator),
                    std::forward<tp_arguments_ts>(p_arguments)...
                ));
            }
        };
    }
    auto constexpr bind_front = detail::bind_front_modifier{};

    namespace detail {
        struct bind_back_modifier : modifier_base<bind_back_modifier> {
            using modifier_base<bind_back_modifier>::operator();
            template <
                combinator tp_combinator_t,
                class...   tp_arguments_ts
            >
            auto constexpr operator()[[nodiscard]](
                tp_combinator_t&&    p_combinator,
                tp_arguments_ts&&... p_arguments
            )
            const noexcept(noexcept(
                identity(std::bind_back(
                    std::declval<tp_combinator_t>(),
                    std::declval<tp_arguments_ts>()...
                ))
            ))
            -> decltype(
                identity(std::bind_back(
                    std::forward<tp_combinator_t>(p_combinator),
                    std::forward<tp_arguments_ts>(p_arguments)...
                ))
            ) {
                return identity(std::bind_back(
                    std::forward<tp_combinator_t>(p_combinator),
                    std::forward<tp_arguments_ts>(p_arguments)...
                ));
            }
        };
    }
    auto constexpr bind_back = detail::bind_back_modifier{};
}

template <
    pfs::detail::adaptor tp_adaptor_t,
    typename             tp_element_t
>
requires (
    std::invocable<
        tp_adaptor_t,
        tp_element_t
    >
)
auto constexpr operator|[[nodiscard]] (
    tp_adaptor_t&& p_adaptor,
    tp_element_t&& p_element
)
noexcept(noexcept(
    std::invoke(
        std::declval<tp_adaptor_t>(),
        std::declval<tp_element_t>()
    )
))
// should be:
// -> decltype(
//     std::invoke(
//         std::forward<tp_adaptor_t>(p_adaptor),
//         std::forward<tp_element_t>(p_element)
//     )
// )
// and without the require's clause, but clangd is broken and shows false errors, so this is a workaround
-> decltype(auto) {
    return std::invoke(
        std::forward<tp_adaptor_t>(p_adaptor),
        std::forward<tp_element_t>(p_element)
    );
}

template <
    pfs::detail::combinator tp_combinator_t,
    typename                tp_element_t
>
auto constexpr operator|[[nodiscard]] (
    tp_combinator_t&& p_combinator,
    tp_element_t&&    p_element
)
noexcept(noexcept(
    pfs::detail::combinator_append_element(
        std::make_index_sequence<pfs::detail::combinator_traits<tp_combinator_t>::m_size>{},
        std::declval<tp_combinator_t>(),
        std::declval<tp_element_t>()
    )
))
-> decltype(
    pfs::detail::combinator_append_element(
        std::make_index_sequence<pfs::detail::combinator_traits<tp_combinator_t>::m_size>{},
        std::forward<tp_combinator_t>(p_combinator),
        std::forward<tp_element_t>(p_element)
    )
) {
    return pfs::detail::combinator_append_element(
        std::make_index_sequence<pfs::detail::combinator_traits<tp_combinator_t>::m_size>{},
        std::forward<tp_combinator_t>(p_combinator),
        std::forward<tp_element_t>(p_element)
    );
}

template <
    pfs::detail::combinator        tp_combinator_t,
    pfs::detail::modifier_closure  tp_modifier_closure_t
>
requires (
    std::invocable<
        tp_modifier_closure_t,
        tp_combinator_t
    >
)
auto constexpr operator+=[[nodiscard]] (
    tp_combinator_t&&        p_combinator,
    tp_modifier_closure_t&&  p_modifier_closure
)
noexcept(noexcept(
    std::invoke(
        std::declval<tp_modifier_closure_t>(),
        std::declval<tp_combinator_t>()
    )
))
-> decltype(
    std::invoke(
        std::forward<tp_modifier_closure_t>(p_modifier_closure),
        std::forward<tp_combinator_t>(p_combinator)
    )
) {
    return std::invoke(
        std::forward<tp_modifier_closure_t>(p_modifier_closure),
        std::forward<tp_combinator_t>(p_combinator)
    );
}

template <
    pfs::detail::modifier_closure tp_modifier_closure1_t,
    pfs::detail::modifier_closure tp_modifier_closure2_t
>
auto constexpr operator|[[nodiscard]] (
    tp_modifier_closure1_t&& p_modifier_closure1,
    tp_modifier_closure2_t&& p_modifier_closure2
)
noexcept(noexcept(
    pfs::detail::pipe_modifier<
        std::remove_cvref_t<tp_modifier_closure1_t>,
        std::remove_cvref_t<tp_modifier_closure2_t>
    >{
        {},
        std::declval<tp_modifier_closure1_t>(),
        std::declval<tp_modifier_closure2_t>()
    }
))
-> decltype(
    pfs::detail::pipe_modifier<
        std::remove_cvref_t<tp_modifier_closure1_t>,
        std::remove_cvref_t<tp_modifier_closure2_t>
    >{
        {},
        std::forward<tp_modifier_closure1_t>(p_modifier_closure1),
        std::forward<tp_modifier_closure2_t>(p_modifier_closure2)
    }
) {
    return pfs::detail::pipe_modifier<
        std::remove_cvref_t<tp_modifier_closure1_t>,
        std::remove_cvref_t<tp_modifier_closure2_t>
    >{
        {},
        std::forward<tp_modifier_closure1_t>(p_modifier_closure1),
        std::forward<tp_modifier_closure2_t>(p_modifier_closure2)
    };
}

template <
    pfs::detail::combinator tp_combinator1_t,
    pfs::detail::combinator tp_combinator2_t
>
auto constexpr operator|[[nodiscard]] (
    tp_combinator1_t&& p_combinator1,
    tp_combinator2_t&& p_combinator2
)
noexcept(noexcept(
    pfs::detail::merge_combinators(
        std::make_index_sequence<pfs::detail::combinator_traits<tp_combinator1_t>::m_size>{},
        std::make_index_sequence<pfs::detail::combinator_traits<tp_combinator2_t>::m_size>{},
        std::declval<tp_combinator1_t>(),
        std::declval<tp_combinator2_t>()
    )
))
-> decltype(
    pfs::detail::merge_combinators(
        std::make_index_sequence<pfs::detail::combinator_traits<tp_combinator1_t>::m_size>{},
        std::make_index_sequence<pfs::detail::combinator_traits<tp_combinator2_t>::m_size>{},
        std::forward<tp_combinator1_t>(p_combinator1),
        std::forward<tp_combinator2_t>(p_combinator2)
    )
) {
    return pfs::detail::merge_combinators(
        std::make_index_sequence<pfs::detail::combinator_traits<tp_combinator1_t>::m_size>{},
        std::make_index_sequence<pfs::detail::combinator_traits<tp_combinator2_t>::m_size>{},
        std::forward<tp_combinator1_t>(p_combinator1),
        std::forward<tp_combinator2_t>(p_combinator2)
    );
}

template <
    typename tp_combinator_or_element1_t,
    typename tp_combinator_or_element2_t
>
requires (
    pfs::detail::combinator<tp_combinator_or_element1_t> ||
    pfs::detail::combinator<tp_combinator_or_element2_t>
)
auto constexpr operator&&[[nodiscard]] (
    tp_combinator_or_element1_t&& p_combinator_or_element1,
    tp_combinator_or_element2_t&& p_combinator_or_element2
)
noexcept(noexcept(
    pfs::all(
        std::declval<tp_combinator_or_element1_t>(),
        std::declval<tp_combinator_or_element2_t>()
    )
))
-> decltype(
    pfs::all(
        std::forward<tp_combinator_or_element1_t>(p_combinator_or_element1),
        std::forward<tp_combinator_or_element2_t>(p_combinator_or_element2)
    )
) {
    return pfs::all(
        std::forward<tp_combinator_or_element1_t>(p_combinator_or_element1),
        std::forward<tp_combinator_or_element2_t>(p_combinator_or_element2)
    );
}

template <
    typename tp_combinator_or_element1_t,
    typename tp_combinator_or_element2_t
>
requires (
    pfs::detail::combinator<tp_combinator_or_element1_t> ||
    pfs::detail::combinator<tp_combinator_or_element2_t>
)
auto constexpr operator||[[nodiscard]] (
    tp_combinator_or_element1_t&& p_combinator_or_element1,
    tp_combinator_or_element2_t&& p_combinator_or_element2
)
noexcept(noexcept(
    pfs::any(
        std::declval<tp_combinator_or_element1_t>(),
        std::declval<tp_combinator_or_element2_t>()
    )
))
-> decltype(
    pfs::any(
        std::forward<tp_combinator_or_element1_t>(p_combinator_or_element1),
        std::forward<tp_combinator_or_element2_t>(p_combinator_or_element2)
    )
) {
    return pfs::any(
        std::forward<tp_combinator_or_element1_t>(p_combinator_or_element1),
        std::forward<tp_combinator_or_element2_t>(p_combinator_or_element2)
    );
}

template <pfs::detail::combinator tp_combinator_t>
struct std::tuple_size<tp_combinator_t> : std::integral_constant<
    std::size_t,
    pfs::detail::combinator_traits<tp_combinator_t>::m_size
> {};

template <
    std::size_t             tp_index,
    pfs::detail::combinator tp_combinator_t
>
struct std::tuple_element<
    tp_index,
    tp_combinator_t
> : std::type_identity<
    typename pfs::detail::combinator_traits<tp_combinator_t>::template m_element_t<tp_index>
> {};

namespace std {
    template <std::size_t tp_index, pfs::detail::combinator_or_modifier_closure tp_combinator_or_modifier_closure_t>
    auto constexpr get[[nodiscard]](tp_combinator_or_modifier_closure_t&& p_combinator_or_modifier_closure)
    noexcept(noexcept(pfs::detail::adl_exposed::get<tp_index>(std::declval<tp_combinator_or_modifier_closure_t>())))
    -> decltype(pfs::detail::adl_exposed::get<tp_index>(std::forward<tp_combinator_or_modifier_closure_t>(p_combinator_or_modifier_closure))) {
        return pfs::detail::adl_exposed::get<tp_index>(std::forward<tp_combinator_or_modifier_closure_t>(p_combinator_or_modifier_closure));
    }
}

#endif
