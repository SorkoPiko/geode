// xhash internal header

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Per Apache License, Version 2.0, Section 4, Point b: I (kynex7510) changed this file.

#ifndef _GEODE_XHASH_
#define _GEODE_XHASH_
#include <yvals_core.h>
#if _STL_COMPILER_PREPROCESSOR
#include <cmath>
#include <list>
#include <tuple>
#include <vector>
#include <xbit_ops.h>

#ifdef _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#include <cstring>
#include <cwchar>
#include <xstring>
#endif // _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS

#if _HAS_CXX17
#include <xnode_handle.h>
#endif // _HAS_CXX17

#pragma pack(push, _CRT_PACKING)
#pragma warning(push, _STL_WARNING_LEVEL)
#pragma warning(disable : _STL_DISABLED_WARNINGS)
_STL_DISABLE_CLANG_WARNINGS
#pragma push_macro("new")
#undef new

// Include the original file for the required structs.
#include <xhash>

namespace geode::stl {

using _STD _Fake_alloc;

// The following types are not accessible from std::list.
template <class _Ty, class _Alloc = _STD allocator<_Ty>>
struct _ListImpl {
    using _Mylist = _STD list<_Ty, _Alloc>;
    using _Alty = _STD _Rebind_alloc_t<_Alloc, _Ty>;
    using _Alty_traits = _STD allocator_traits<_Alty>;
    using _Node = _STD _List_node<_Ty, typename _STD allocator_traits<_Alloc>::void_pointer>;
    using _Alnode = _STD _Rebind_alloc_t<_Alloc, _Node>;
    using _Alnode_traits = _STD allocator_traits<_Alnode>;
    using _Nodeptr = typename _Alnode_traits::pointer;

     using _Val_types = _STD conditional_t<_STD _Is_simple_alloc_v<_Alnode>, _STD _List_simple_types<_Ty>,
        _STD _List_iter_types<_Ty, typename _Alty_traits::size_type, typename _Alty_traits::difference_type,
            typename _Alty_traits::pointer, typename _Alty_traits::const_pointer, _Nodeptr>>;

    using _Scary_val = _STD _List_val<_Val_types>;

    using _PairType = _STD _Compressed_pair<_Alnode, _Scary_val>;

    // These methods are required to bypass std::list visibility.

    constexpr static size_t MYPAIR_OFFSET = 0;

    static _PairType& _List_Mypair(_Mylist& _List) noexcept {
        return *reinterpret_cast<_PairType*>(reinterpret_cast<uintptr_t>(&_List) + MYPAIR_OFFSET);
    }

    static const _PairType& _List_Mypair(const _Mylist& _List) noexcept {
        return *reinterpret_cast<_PairType*>(reinterpret_cast<uintptr_t>(&_List) + MYPAIR_OFFSET);
    }

    static _Alnode& _List_Getal(_Mylist& _List) noexcept {
        return _List_Mypair(_List)._Get_first();
    }

    static const _Alnode& _List_Getal(const _Mylist& _List) noexcept {
        return _List_Mypair(_List)._Get_first();
    }
};

template <class _Traits>
class _Hash { // hash table -- list with vector of iterators for quick access
protected:
    using _MylistImpl         = typename _ListImpl<typename _Traits::value_type, typename _Traits::allocator_type>;
    using _Mylist             = typename _MylistImpl::_Mylist;
    using _Alnode             = typename /* _Mylist */ _MylistImpl::_Alnode;
    using _Alnode_traits      = typename /* _Mylist */ _MylistImpl::_Alnode_traits;
    using _Node               = typename /* _Mylist */ _MylistImpl::_Node;
    using _Nodeptr            = typename /* _Mylist */ _MylistImpl::_Nodeptr;
    using _Mutable_value_type = typename _Traits::_Mutable_value_type;

    using _Key_compare   = typename _Traits::key_compare;
    using _Value_compare = typename _Traits::value_compare;

    typename _MylistImpl::_PairType& _List_Mypair() noexcept { return _MylistImpl::_List_Mypair(_List); }
    const typename _MylistImpl::_PairType& _List_Mypair() const noexcept { return _MylistImpl::_List_Mypair(_List); }

    _Alnode& _List_Getal() noexcept { return _MylistImpl::_List_Getal(_List); }
    const _Alnode& _List_Getal() const noexcept { return _MylistImpl::_List_Getal(_List); }

public:
    using key_type = typename _Traits::key_type;

    using value_type      = typename _Mylist::value_type;
    using allocator_type  = typename _Mylist::allocator_type;
    using size_type       = typename _Mylist::size_type;
    using difference_type = typename _Mylist::difference_type;
    using pointer         = typename _Mylist::pointer;
    using const_pointer   = typename _Mylist::const_pointer;
    using reference       = value_type&;
    using const_reference = const value_type&;

    using iterator =
        _STD conditional_t<_STD is_same_v<key_type, value_type>, typename _Mylist::const_iterator, typename _Mylist::iterator>;
    using const_iterator = typename _Mylist::const_iterator;

    using _Unchecked_iterator       = _STD conditional_t<_STD is_same_v<key_type, value_type>,
        typename _Mylist::_Unchecked_const_iterator, typename _Mylist::_Unchecked_iterator>;
    using _Unchecked_const_iterator = typename _Mylist::_Unchecked_const_iterator;

    using _Aliter = _STD _Rebind_alloc_t<_Alnode, _Unchecked_iterator>;

    static constexpr size_type _Bucket_size = _Key_compare::bucket_size;
    static constexpr size_type _Min_buckets = 8; // must be a positive power of 2
    static constexpr bool _Multi            = _Traits::_Multi;

    template <class _TraitsT>
    friend bool _Hash_equal(const _Hash<_TraitsT>& _Left, const _Hash<_TraitsT>& _Right);

protected:
    _Hash(const _Key_compare& _Parg, const allocator_type& _Al)
        : _Traitsobj(_Parg), _List(_Al), _Vec(_Al), _Mask(_Min_buckets - 1), _Maxidx(_Min_buckets) {
        // construct empty hash table
        _Max_bucket_size() = _Bucket_size;
        _Vec._Assign_grow(_Min_buckets * 2, _List._Unchecked_end());
#ifdef _ENABLE_STL_INTERNAL_CHECK
        _Stl_internal_check_container_invariants();
#endif // _ENABLE_STL_INTERNAL_CHECK
    }

    template <class _Any_alloc>
    _Hash(const _Hash& _Right, const _Any_alloc& _Al)
        : _Traitsobj(_Right._Traitsobj), _List(static_cast<allocator_type>(_Al)), _Vec(_Al), _Mask(_Right._Mask),
          _Maxidx(_Right._Maxidx) {
        // construct hash table by copying _Right
        _Vec._Assign_grow(_Right._Vec.size(), _List._Unchecked_end());
        _Insert_range_unchecked(_Right._Unchecked_begin(), _Right._Unchecked_end());
#ifdef _ENABLE_STL_INTERNAL_CHECK
        _Stl_internal_check_container_invariants();
        _Right._Stl_internal_check_container_invariants();
#endif // _ENABLE_STL_INTERNAL_CHECK
    }

    _Hash(_Hash&& _Right)
        : _Traitsobj(_Right._Traitsobj), _List(_STD _Move_allocator_tag{}, _Right._List_Getal()),
          _Vec(_STD move(_Right._Vec._Mypair._Get_first())) {
        _Vec._Assign_grow(_Min_buckets * 2, _Unchecked_end());
        _List._Swap_val(_Right._List);
        _Vec._Mypair._Myval2._Swap_val(_Right._Vec._Mypair._Myval2);
        _Mask   = _STD exchange(_Right._Mask, _Min_buckets - 1);
        _Maxidx = _STD exchange(_Right._Maxidx, _Min_buckets);
#ifdef _ENABLE_STL_INTERNAL_CHECK
        _Stl_internal_check_container_invariants();
        _Right._Stl_internal_check_container_invariants();
#endif // _ENABLE_STL_INTERNAL_CHECK
    }

private:
    void _Move_construct_equal_alloc(_Hash& _Right) {
        _Vec._Assign_grow(_Min_buckets * 2, _Unchecked_end());
        _List._Swap_val(_Right._List);
        _Vec._Mypair._Myval2._Swap_val(_Right._Vec._Mypair._Myval2);
        _Mask   = _STD exchange(_Right._Mask, _Min_buckets - 1);
        _Maxidx = _STD exchange(_Right._Maxidx, _Min_buckets);
    }

public:
    _Hash(_Hash&& _Right, const allocator_type& _Al) : _Traitsobj(_Right._Traitsobj), _List(_Al), _Vec(_Al) {
        // construct hash table by moving _Right, allocator
        if constexpr (_Alnode_traits::is_always_equal::value) {
            _Move_construct_equal_alloc(_Right);
        } else if (_List_Getal() == _Right._List_Getal()) {
            _Move_construct_equal_alloc(_Right);
        } else {
            _Maxidx            = _Min_buckets;
            const auto _Myhead = _List_Mypair()._Myval2._Myhead;
            for (auto& _Val : _Right._List) {
                _List._Emplace(_Myhead, reinterpret_cast<_Mutable_value_type&&>(_Val));
            }
            _Reinsert_with_invalid_vec();
            _Right.clear();
        }

#ifdef _ENABLE_STL_INTERNAL_CHECK
        _Stl_internal_check_container_invariants();
        _Right._Stl_internal_check_container_invariants();
#endif // _ENABLE_STL_INTERNAL_CHECK
    }

private:
    void _Swap_val(_Hash& _Right) noexcept { // swap contents with equal allocator _Hash _Right
        _List._Swap_val(_Right._List);
        _Vec._Mypair._Myval2._Swap_val(_Right._Vec._Mypair._Myval2);
        _STD swap(_Mask, _Right._Mask);
        _STD swap(_Maxidx, _Right._Maxidx);
    }

    struct _Min_buckets_construct_ptr {
        using pointer = typename _STD allocator_traits<_Aliter>::pointer;
        _Aliter& _Al;
        pointer _Base;
        _Min_buckets_construct_ptr(_Aliter& _Al_) : _Al(_Al_), _Base(_Al.allocate(_Min_buckets * 2)) {}
        _Min_buckets_construct_ptr(const _Min_buckets_construct_ptr&) = delete;
        _NODISCARD pointer _Release(_Unchecked_iterator _Newend) noexcept {
            _STD uninitialized_fill(_Base, _Base + _Min_buckets * 2, _Newend);
            return _STD exchange(_Base, nullptr);
        }
        ~_Min_buckets_construct_ptr() {
            if (_Base) {
                _Al.deallocate(_Base, _Min_buckets * 2);
            }
        }
    };

    void _Pocma_both(_Hash& _Right) {
        _Pocma(_List_Getal(), _Right._List_Getal());
        _Pocma(_Vec._Mypair._Get_first(), _Right._Vec._Mypair._Get_first());
    }

    struct _NODISCARD _Clear_guard {
        _Hash* _Target;

        explicit _Clear_guard(_Hash* const _Target_) : _Target(_Target_) {}

        _Clear_guard(const _Clear_guard&)            = delete;
        _Clear_guard& operator=(const _Clear_guard&) = delete;

        ~_Clear_guard() {
            if (_Target) {
                _Target->clear();
            }
        }
    };

#ifdef _ENABLE_STL_INTERNAL_CHECK
    struct _NODISCARD _Check_container_invariants_guard {
        const _Hash& _Target;

        explicit _Check_container_invariants_guard(const _Hash& _Target_) : _Target(_Target_) {}

        _Check_container_invariants_guard(const _Check_container_invariants_guard&)            = delete;
        _Check_container_invariants_guard& operator=(const _Check_container_invariants_guard&) = delete;

        ~_Check_container_invariants_guard() {
            _Target._Stl_internal_check_container_invariants();
        }
    };
#endif // _ENABLE_STL_INTERNAL_CHECK

public:
    _Hash& operator=(_Hash&& _Right) { // assign by moving _Right
        if (this == _STD addressof(_Right)) {
            return *this;
        }

#ifdef _ENABLE_STL_INTERNAL_CHECK
        _Check_container_invariants_guard _Check_self{*this};
        _Check_container_invariants_guard _Check_right{_Right};
#endif // _ENABLE_STL_INTERNAL_CHECK

        auto& _Al                 = _Getal();
        auto& _Right_al           = _Right._Getal();
        constexpr auto _Pocma_val = _STD _Choose_pocma_v<_Alnode>;
        if constexpr (_Pocma_val == _STD _Pocma_values::_Propagate_allocators) {
            if (_Al != _Right_al) {
                // allocate all the parts necessary to maintain _Hash invariants using _Right's allocator
                auto&& _Alproxy       = _GET_PROXY_ALLOCATOR(_Alnode, _Al);
                auto&& _Right_alproxy = _GET_PROXY_ALLOCATOR(_Alnode, _Right_al);
                _STD _Container_proxy_ptr<_Alnode> _List_proxy(_Right_alproxy, _STD _Leave_proxy_unbound{});
                _STD _Container_proxy_ptr<_Alnode> _Vec_proxy(_Right_alproxy, _STD _Leave_proxy_unbound{});
                _STD _List_head_construct_ptr<_Alnode> _Newhead(_Right_al);
                _Min_buckets_construct_ptr _Buckets(_Right._Vec._Mypair._Get_first());

                // assign the hash/compare ops; we have no control over whether this throws, and if it does we want
                // to do nothing
                _Traitsobj = _Right._Traitsobj;

                // nothrow hereafter

                // release any state we are currently owning, and propagate the allocators
                _List._Tidy();
                _Vec._Tidy();
                _Pocma_both(_Right);

                // assign the empty list to _Right._List (except the allocators), and take _Right's _List data
                auto& _List_data       = _List_Mypair()._Myval2;
                auto& _Right_list_data = _Right._List_Mypair()._Myval2;
                _List_data._Myhead     = _STD exchange(_Right_list_data._Myhead, _Newhead._Release());
                _List_data._Mysize     = _STD exchange(_Right_list_data._Mysize, size_type{0});
                _List_proxy._Bind(_Alproxy, _STD addressof(_List_data));
                _List_data._Swap_proxy_and_iterators(_Right_list_data);

                // assign the _Min_buckets into _Right's _Vec data and take _Right's _Vec data
                auto& _Vec_data       = _Vec._Mypair._Myval2;
                auto& _Right_vec_data = _Right._Vec._Mypair._Myval2;

                const auto _Newfirst = _Buckets._Release(_Right._Unchecked_end());
                const auto _Newlast  = _Newfirst + _Min_buckets * 2;

                _Vec_data._Myfirst = _STD exchange(_Right_vec_data._Myfirst, _Newfirst);
                _Vec_data._Mylast  = _STD exchange(_Right_vec_data._Mylast, _Newlast);
                _Vec_data._Myend   = _STD exchange(_Right_vec_data._Myend, _Newlast);
                _Vec_proxy._Bind(_Alproxy, _STD addressof(_Vec_data));
                _Vec_data._Swap_proxy_and_iterators(_Right_vec_data);

                // give _Right the default _Mask and _Maxidx values and take its former values
                _Mask   = _STD exchange(_Right._Mask, _Min_buckets - 1);
                _Maxidx = _STD exchange(_Right._Maxidx, _Min_buckets);

                return *this;
            }
        } else if constexpr (_Pocma_val == _STD _Pocma_values::_No_propagate_allocators) {
            if (_Al != _Right_al) {
                _Clear_guard _Guard{this};
                _Traitsobj     = _Right._Traitsobj;
                using _Adapter = _STD _Reinterpret_move_iter<typename _Mylist::_Unchecked_iterator, _Mutable_value_type>;
                _List.template _Assign_cast<_Mutable_value_type&>(
                    _Adapter{_Right._List._Unchecked_begin()}, _Adapter{_Right._List._Unchecked_end()});
                _Reinsert_with_invalid_vec();
                _Guard._Target = nullptr;

                return *this;
            }
        }

        clear();
        _Traitsobj = _Right._Traitsobj;
        _Pocma_both(_Right);
        _Swap_val(_Right);

        return *this;
    }

    template <class... _Valtys>
    _STD conditional_t<_Multi, iterator, _STD pair<iterator, bool>> emplace(_Valtys&&... _Vals) {
        // try to insert value_type(_Vals...)
        using _In_place_key_extractor = typename _Traits::template _In_place_key_extractor<_STD _Remove_cvref_t<_Valtys>...>;
        if constexpr (_Multi) {
            _Check_max_size();
            _STD _List_node_emplace_op2<_Alnode> _Newnode(_List_Getal(), _STD forward<_Valtys>(_Vals)...);
            const auto& _Keyval = _Traits::_Kfn(_Newnode._Ptr->_Myval);
            const auto _Hashval = _Traitsobj(_Keyval);
            if (_Check_rehash_required_1()) {
                _Rehash_for_1();
            }

            const auto _Target = _Find_last(_Keyval, _Hashval);
            return _List._Make_iter(_Insert_new_node_before(_Hashval, _Target._Insert_before, _Newnode._Release()));
        } else if constexpr (_In_place_key_extractor::_Extractable) {
            const auto& _Keyval = _In_place_key_extractor::_Extract(_Vals...);
            const auto _Hashval = _Traitsobj(_Keyval);
            auto _Target        = _Find_last(_Keyval, _Hashval);
            if (_Target._Duplicate) {
                return {_List._Make_iter(_Target._Duplicate), false};
            }

            _Check_max_size();
            // invalidates _Keyval:
            _STD _List_node_emplace_op2<_Alnode> _Newnode(_List_Getal(), _STD forward<_Valtys>(_Vals)...);
            if (_Check_rehash_required_1()) {
                _Rehash_for_1();
                _Target = _Find_last(_Traits::_Kfn(_Newnode._Ptr->_Myval), _Hashval);
            }

            return {
                _List._Make_iter(_Insert_new_node_before(_Hashval, _Target._Insert_before, _Newnode._Release())), true};
        } else {
            _STD _List_node_emplace_op2<_Alnode> _Newnode(_List_Getal(), _STD forward<_Valtys>(_Vals)...);
            const auto& _Keyval = _Traits::_Kfn(_Newnode._Ptr->_Myval);
            const auto _Hashval = _Traitsobj(_Keyval);
            auto _Target        = _Find_last(_Keyval, _Hashval);
            if (_Target._Duplicate) {
                return {_List._Make_iter(_Target._Duplicate), false};
            }

            _Check_max_size();
            if (_Check_rehash_required_1()) {
                _Rehash_for_1();
                _Target = _Find_last(_Traits::_Kfn(_Newnode._Ptr->_Myval), _Hashval);
            }

            return {
                _List._Make_iter(_Insert_new_node_before(_Hashval, _Target._Insert_before, _Newnode._Release())), true};
        }
    }

    template <class... _Valtys>
    iterator emplace_hint(const_iterator _Hint, _Valtys&&... _Vals) { // try to insert value_type(_Vals...)
        using _In_place_key_extractor = typename _Traits::template _In_place_key_extractor<_STD _Remove_cvref_t<_Valtys>...>;
        if constexpr (_Multi) {
            _Check_max_size();
            _STD _List_node_emplace_op2<_Alnode> _Newnode(_List_Getal(), _STD forward<_Valtys>(_Vals)...);
            const auto& _Keyval = _Traits::_Kfn(_Newnode._Ptr->_Myval);
            const auto _Hashval = _Traitsobj(_Keyval);
            if (_Check_rehash_required_1()) {
                _Rehash_for_1();
            }

            const auto _Target = _Find_hint(_Hint._Ptr, _Keyval, _Hashval);
            return _List._Make_iter(_Insert_new_node_before(_Hashval, _Target._Insert_before, _Newnode._Release()));
        } else if constexpr (_In_place_key_extractor::_Extractable) {
            const auto& _Keyval = _In_place_key_extractor::_Extract(_Vals...);
            const auto _Hashval = _Traitsobj(_Keyval);
            auto _Target        = _Find_hint(_Hint._Ptr, _Keyval, _Hashval);
            if (_Target._Duplicate) {
                return _List._Make_iter(_Target._Duplicate);
            }

            _Check_max_size();
            // invalidates _Keyval:
            _STD _List_node_emplace_op2<_Alnode> _Newnode(_List_Getal(), _STD forward<_Valtys>(_Vals)...);
            if (_Check_rehash_required_1()) {
                _Rehash_for_1();
                _Target = _Find_hint(_Hint._Ptr, _Traits::_Kfn(_Newnode._Ptr->_Myval), _Hashval);
            }

            return _List._Make_iter(_Insert_new_node_before(_Hashval, _Target._Insert_before, _Newnode._Release()));
        } else {
            _STD _List_node_emplace_op2<_Alnode> _Newnode(_List_Getal(), _STD forward<_Valtys>(_Vals)...);
            const auto& _Keyval = _Traits::_Kfn(_Newnode._Ptr->_Myval);
            const auto _Hashval = _Traitsobj(_Keyval);
            auto _Target        = _Find_hint(_Hint._Ptr, _Keyval, _Hashval);
            if (_Target._Duplicate) {
                return _List._Make_iter(_Target._Duplicate);
            }

            _Check_max_size();
            if (_Check_rehash_required_1()) {
                _Rehash_for_1();
                _Target = _Find_hint(_Hint._Ptr, _Traits::_Kfn(_Newnode._Ptr->_Myval), _Hashval);
            }

            return _List._Make_iter(_Insert_new_node_before(_Hashval, _Target._Insert_before, _Newnode._Release()));
        }
    }

protected:
    template <class _Keyty, class... _Mappedty>
    _STD pair<_Nodeptr, bool> _Try_emplace(_Keyty&& _Keyval_arg, _Mappedty&&... _Mapval) {
        const auto& _Keyval = _Keyval_arg;
        const auto _Hashval = _Traitsobj(_Keyval);
        auto _Target        = _Find_last(_Keyval, _Hashval);
        if (_Target._Duplicate) {
            return {_Target._Duplicate, false};
        }

        _Check_max_size();
        _STD _List_node_emplace_op2<_Alnode> _Newnode(_List_Getal(), _STD piecewise_construct,
            _STD forward_as_tuple(_STD forward<_Keyty>(_Keyval_arg)),
            _STD forward_as_tuple(_STD forward<_Mappedty>(_Mapval)...));
        if (_Check_rehash_required_1()) {
            _Rehash_for_1();
            _Target = _Find_last(_Traits::_Kfn(_Newnode._Ptr->_Myval), _Hashval);
        }

        return {_Insert_new_node_before(_Hashval, _Target._Insert_before, _Newnode._Release()), true};
    }

    template <class _Keyty, class... _Mappedty>
    _Nodeptr _Try_emplace_hint(const _Nodeptr _Hint, _Keyty&& _Keyval_arg, _Mappedty&&... _Mapval) {
        const auto& _Keyval = _Keyval_arg;
        const auto _Hashval = _Traitsobj(_Keyval);
        auto _Target        = _Find_hint(_Hint, _Keyval, _Hashval);
        if (_Target._Duplicate) {
            return _Target._Duplicate;
        }

        _Check_max_size();
        // might invalidate _Keyval:
        _STD _List_node_emplace_op2<_Alnode> _Newnode(_List_Getal(), _STD piecewise_construct,
            _STD forward_as_tuple(_STD forward<_Keyty>(_Keyval_arg)),
            _STD forward_as_tuple(_STD forward<_Mappedty>(_Mapval)...));
        if (_Check_rehash_required_1()) {
            _Rehash_for_1();
            _Target = _Find_hint(_Hint, _Traits::_Kfn(_Newnode._Ptr->_Myval), _Hashval);
        }

        return _Insert_new_node_before(_Hashval, _Target._Insert_before, _Newnode._Release());
    }

private:
    void _Pocca_both(const _Hash& _Right) {
        _Pocca(_List_Getal(), _Right._List_Getal());
        _Pocca(_Vec._Mypair._Get_first(), _Right._Vec._Mypair._Get_first());
    }

public:
    _Hash& operator=(const _Hash& _Right) {
        if (this == _STD addressof(_Right)) {
            return *this;
        }

#ifdef _ENABLE_STL_INTERNAL_CHECK
        _Check_container_invariants_guard _Check_self{*this};
        _Check_container_invariants_guard _Check_right{_Right};
#endif // _ENABLE_STL_INTERNAL_CHECK

        if constexpr (_STD _Choose_pocca_v<_Alnode>) {
            auto& _Al             = _Getal();
            const auto& _Right_al = _Right._Getal();
            if (_Al != _Right_al) {
                auto&& _Alproxy       = _GET_PROXY_ALLOCATOR(_Alnode, _Al);
                auto&& _Right_alproxy = _GET_PROXY_ALLOCATOR(_Alnode, _Right_al);
                _STD _Container_proxy_ptr<_Alnode> _Vec_proxy(_Right_alproxy, _STD _Leave_proxy_unbound{});
                _List._Reload_sentinel_and_proxy(_Right._List);
                _Vec._Tidy();
                _Pocca_both(_Right);
                _Vec_proxy._Bind(_Alproxy, _STD addressof(_Vec._Mypair._Myval2));

                _Clear_guard _Guard{this};
                _Traitsobj = _Right._Traitsobj;
                _List.template _Assign_cast<_Mutable_value_type&>(
                    _Right._List._Unchecked_begin(), _Right._List._Unchecked_end());
                _Reinsert_with_invalid_vec();
                _Guard._Target = nullptr;

                return *this;
            }
        }

        _Clear_guard _Guard{this};
        _Traitsobj = _Right._Traitsobj;
        _Pocca_both(_Right);
        _List.template _Assign_cast<_Mutable_value_type&>(
            _Right._List._Unchecked_begin(), _Right._List._Unchecked_end());
        _Reinsert_with_invalid_vec();
        _Guard._Target = nullptr;

        return *this;
    }

    _NODISCARD iterator begin() noexcept {
        return _List.begin();
    }

    _NODISCARD const_iterator begin() const noexcept {
        return _List.begin();
    }

    _NODISCARD iterator end() noexcept {
        return _List.end();
    }

    _NODISCARD const_iterator end() const noexcept {
        return _List.end();
    }

    _Unchecked_iterator _Unchecked_begin() noexcept {
        return _List._Unchecked_begin();
    }

    _Unchecked_const_iterator _Unchecked_begin() const noexcept {
        return _List._Unchecked_begin();
    }

    _Unchecked_iterator _Unchecked_end() noexcept {
        return _List._Unchecked_end();
    }

    _Unchecked_const_iterator _Unchecked_end() const noexcept {
        return _List._Unchecked_end();
    }

    _NODISCARD const_iterator cbegin() const noexcept {
        return begin();
    }

    _NODISCARD const_iterator cend() const noexcept {
        return end();
    }

    _NODISCARD size_type size() const noexcept {
        return _List.size();
    }

    _NODISCARD size_type max_size() const noexcept {
        return _List.max_size();
    }

    _NODISCARD_EMPTY_MEMBER bool empty() const noexcept {
        return _List.empty();
    }

    _NODISCARD allocator_type get_allocator() const noexcept {
        return static_cast<allocator_type>(_List.get_allocator());
    }

    using local_iterator       = iterator;
    using const_local_iterator = const_iterator;

    _NODISCARD size_type bucket_count() const noexcept {
        return _Maxidx;
    }

    _NODISCARD size_type max_bucket_count() const noexcept {
        return _Vec.max_size() >> 1;
    }

    _NODISCARD size_type bucket(const key_type& _Keyval) const
        noexcept(_STD _Nothrow_hash<_Traits, key_type>) /* strengthened */ {
        return _Traitsobj(_Keyval) & _Mask;
    }

    _NODISCARD size_type bucket_size(size_type _Bucket) const noexcept /* strengthened */ {
        _Unchecked_iterator _Bucket_lo = _Vec._Mypair._Myval2._Myfirst[_Bucket << 1];
        if (_Bucket_lo == _Unchecked_end()) {
            return 0;
        }

        return static_cast<size_type>(_STD distance(_Bucket_lo, _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1])) + 1;
    }

    _NODISCARD local_iterator begin(size_type _Bucket) noexcept /* strengthened */ {
        return _List._Make_iter(_Vec._Mypair._Myval2._Myfirst[_Bucket << 1]._Ptr);
    }

    _NODISCARD const_local_iterator begin(size_type _Bucket) const noexcept /* strengthened */ {
        return _List._Make_const_iter(_Vec._Mypair._Myval2._Myfirst[_Bucket << 1]._Ptr);
    }

    _NODISCARD local_iterator end(size_type _Bucket) noexcept /* strengthened */ {
        _Nodeptr _Bucket_hi = _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1]._Ptr;
        if (_Bucket_hi != _List_Mypair()._Myval2._Myhead) {
            _Bucket_hi = _Bucket_hi->_Next;
        }

        return _List._Make_iter(_Bucket_hi);
    }

    _NODISCARD const_local_iterator end(size_type _Bucket) const noexcept /* strengthened */ {
        _Nodeptr _Bucket_hi = _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1]._Ptr;
        if (_Bucket_hi != _List_Mypair()._Myval2._Myhead) {
            _Bucket_hi = _Bucket_hi->_Next;
        }

        return _List._Make_const_iter(_Bucket_hi);
    }

    _NODISCARD const_local_iterator cbegin(size_type _Bucket) const noexcept /* strengthened */ {
        return _List._Make_const_iter(_Vec._Mypair._Myval2._Myfirst[_Bucket << 1]._Ptr);
    }

    _NODISCARD const_local_iterator cend(size_type _Bucket) const noexcept /* strengthened */ {
        _Nodeptr _Bucket_hi = _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1]._Ptr;
        if (_Bucket_hi != _List_Mypair()._Myval2._Myhead) {
            _Bucket_hi = _Bucket_hi->_Next;
        }

        return _List._Make_const_iter(_Bucket_hi);
    }

    _NODISCARD float load_factor() const noexcept {
        return static_cast<float>(size()) / static_cast<float>(bucket_count());
    }

    _NODISCARD float max_load_factor() const noexcept {
        return _Max_bucket_size();
    }

    void max_load_factor(float _Newmax) noexcept /* strengthened */ {
        _STL_ASSERT(!(_CSTD isnan)(_Newmax) && _Newmax > 0, "invalid hash load factor");
        _Max_bucket_size() = _Newmax;
    }

    void rehash(size_type _Buckets) { // rebuild table with at least _Buckets buckets
        // don't violate a.bucket_count() >= a.size() / a.max_load_factor() invariant:
        _Buckets = (_STD max)(_Min_load_factor_buckets(_List.size()), _Buckets);
        if (_Buckets <= _Maxidx) { // we already have enough buckets; nothing to do
            return;
        }

        _Forced_rehash(_Buckets);
    }

    void reserve(size_type _Maxcount) { // rebuild table with room for _Maxcount elements
        rehash(_Min_load_factor_buckets(_Maxcount));
    }

    _STD conditional_t<_Multi, iterator, _STD pair<iterator, bool>> insert(const value_type& _Val) {
        return emplace(_Val);
    }

    _STD conditional_t<_Multi, iterator, _STD pair<iterator, bool>> insert(value_type&& _Val) {
        return emplace(_STD move(_Val));
    }

    iterator insert(const_iterator _Hint, const value_type& _Val) {
        return emplace_hint(_Hint, _Val);
    }

    iterator insert(const_iterator _Hint, value_type&& _Val) {
        return emplace_hint(_Hint, _STD move(_Val));
    }

protected:
    template <class _Iter, class _Sent>
    void _Insert_range_unchecked(_Iter _First, const _Sent _Last) {
        for (; _First != _Last; ++_First) {
            emplace(*_First);
        }
    }

public:
    template <class _Iter>
    void insert(_Iter _First, _Iter _Last) {
        _STD _Adl_verify_range(_First, _Last);
        _Insert_range_unchecked(_Get_unwrapped(_First), _Get_unwrapped(_Last));
    }

#if _HAS_CXX23 && defined(__cpp_lib_concepts) // TRANSITION, GH-395
    template <_Container_compatible_range<value_type> _Rng>
    void insert_range(_Rng&& _Range) {
        _Insert_range_unchecked(_RANGES _Ubegin(_Range), _RANGES _Uend(_Range));
    }
#endif // _HAS_CXX23 && defined(__cpp_lib_concepts)

    void insert(_STD initializer_list<value_type> _Ilist) {
        _Insert_range_unchecked(_Ilist.begin(), _Ilist.end());
    }

private:
    _Nodeptr _Unchecked_erase(_Nodeptr _Plist) noexcept(_STD _Nothrow_hash<_Traits, key_type>) {
        size_type _Bucket = bucket(_Traits::_Kfn(_Plist->_Myval));
        _Erase_bucket(_Plist, _Bucket);
        return _List._Unchecked_erase(_Plist);
    }

    struct _Range_eraser {
        _Range_eraser(const _Range_eraser&)            = delete;
        _Range_eraser& operator=(const _Range_eraser&) = delete;

#if _ITERATOR_DEBUG_LEVEL == 2
        // Keep the list nodes around until we have found all those we will erase, for an O(iterators + erasures) update
        // of the iterator chain.
        _Range_eraser(_Mylist& _List_, const _Nodeptr _First_) noexcept
            : _List(_List_), _First(_First_), _Next(_First_) {}

        void _Bump_erased() noexcept {
            _Next = _Next->_Next;
        }

        ~_Range_eraser() noexcept {
            _List._Unchecked_erase(_First, _Next);
        }

        _Mylist& _List;
        const _Nodeptr _First;
        _Nodeptr _Next;
#else // ^^^ _ITERATOR_DEBUG_LEVEL == 2 / _ITERATOR_DEBUG_LEVEL != 2 vvv
      // Destroy the nodes as we encounter them to avoid a second traversal of the linked list.
        _Range_eraser(_Mylist& _List_, const _Nodeptr _First_) noexcept
            : _List(_List_), _Predecessor(_First_->_Prev), _Next(_First_) {}

        void _Bump_erased() noexcept {
            const auto _Oldnext = _Next;
            _Next               = _Oldnext->_Next;
            _Node::_Freenode(_MylistImpl::_List_Getal(_List), _Oldnext);
            --_MylistImpl::_List_Mypair(_List)._Myval2._Mysize;
        }

        ~_Range_eraser() noexcept {
            _Predecessor->_Next = _Next;
            _Next->_Prev        = _Predecessor;
        }

        _Mylist& _List;
        const _Nodeptr _Predecessor;
        _Nodeptr _Next;
#endif
    };

    _Nodeptr _Unchecked_erase(_Nodeptr _First, const _Nodeptr _Last) noexcept(_STD _Nothrow_hash<_Traits, key_type>) {
        if (_First == _Last) {
            return _Last;
        }

        const auto _End           = _List_Mypair()._Myval2._Myhead;
        const auto _Bucket_bounds = _Vec._Mypair._Myval2._Myfirst;
        _Range_eraser _Eraser{_List, _First};
        {
            // process the first bucket, which is special because here _First might not be the beginning of the bucket
            const auto _Predecessor = _First->_Prev;
            const size_type _Bucket = bucket(_Traits::_Kfn(_Eraser._Next->_Myval)); // throws
            // nothrow hereafter this block
            _Nodeptr& _Bucket_lo   = _Bucket_bounds[_Bucket << 1]._Ptr;
            _Nodeptr& _Bucket_hi   = _Bucket_bounds[(_Bucket << 1) + 1]._Ptr;
            const bool _Update_lo  = _Bucket_lo == _Eraser._Next;
            const _Nodeptr _Old_hi = _Bucket_hi;
            for (;;) { // remove elements until we hit the end of the bucket
                const bool _At_bucket_back = _Eraser._Next == _Old_hi;
                _Eraser._Bump_erased();
                if (_At_bucket_back) {
                    break;
                }

                if (_Eraser._Next == _Last) {
                    if (_Update_lo) {
                        // erased the bucket's prefix
                        _Bucket_lo = _Eraser._Next;
                    }

                    return _Last;
                }
            }

            if (_Update_lo) {
                // emptied the bucket
                _Bucket_lo = _End;
                _Bucket_hi = _End;
            } else {
                _Bucket_hi = _Predecessor;
            }
        }

        // hereafter we are always erasing buckets' prefixes
        while (_Eraser._Next != _Last) {
            const size_type _Bucket = bucket(_Traits::_Kfn(_Eraser._Next->_Myval)); // throws
            // nothrow hereafter this block
            _Nodeptr& _Bucket_lo   = _Bucket_bounds[_Bucket << 1]._Ptr;
            _Nodeptr& _Bucket_hi   = _Bucket_bounds[(_Bucket << 1) + 1]._Ptr;
            const _Nodeptr _Old_hi = _Bucket_hi;
            for (;;) { // remove elements until we hit the end of the bucket
                const bool _At_bucket_back = _Eraser._Next == _Old_hi;
                _Eraser._Bump_erased();
                if (_At_bucket_back) {
                    break;
                }

                if (_Eraser._Next == _Last) {
                    // erased the bucket's prefix
                    _Bucket_lo = _Eraser._Next;
                    return _Last;
                }
            }

            // emptied the bucket
            _Bucket_lo = _End;
            _Bucket_hi = _End;
        }

        return _Last;
    }

    template <class _Kx>
    static constexpr bool _Noexcept_heterogeneous_erasure() {
        return _STD _Nothrow_hash<_Traits, _Kx>
            && (!_Multi || (_STD _Nothrow_compare<_Traits, key_type, _Kx> && _STD _Nothrow_compare<_Traits, _Kx, key_type>) );
    }

    template <class _Keytype>
    size_type _Erase(const _Keytype& _Keyval) noexcept(_Noexcept_heterogeneous_erasure<_Keytype>()) /* strengthened */ {
        const size_t _Hashval = _Traitsobj(_Keyval);
        if constexpr (_Multi) {
            const auto _Where = _Equal_range(_Keyval, _Hashval);
            _Unchecked_erase(_Where._First._Ptr, _Where._Last._Ptr);
            return _Where._Distance;
        } else {
            const auto _Target = _Find_last(_Keyval, _Hashval)._Duplicate;
            if (_Target) {
                _Erase_bucket(_Target, _Hashval & _Mask);
                _List._Unchecked_erase(_Target);
                return 1;
            }

            return 0;
        }
    }

public:
    template <class _Iter = iterator, _STD enable_if_t<!_STD is_same_v<_Iter, const_iterator>, int> = 0>
    iterator erase(iterator _Plist) noexcept(_STD _Nothrow_hash<_Traits, key_type>) /* strengthened */ {
        return _List._Make_iter(_Unchecked_erase(_Plist._Ptr));
    }

    iterator erase(const_iterator _Plist) noexcept(_STD _Nothrow_hash<_Traits, key_type>) /* strengthened */ {
        return _List._Make_iter(_Unchecked_erase(_Plist._Ptr));
    }

    iterator erase(const_iterator _First, const_iterator _Last) noexcept(
        _STD _Nothrow_hash<_Traits, key_type>) /* strengthened */ {
        return _List._Make_iter(_Unchecked_erase(_First._Ptr, _Last._Ptr));
    }

    size_type erase(const key_type& _Keyval) noexcept(noexcept(_Erase(_Keyval))) /* strengthened */ {
        return _Erase(_Keyval);
    }

#if _HAS_CXX23
    template <class _Kx, class _Mytraits = _Traits,
        _STD enable_if_t<_Mytraits::template _Supports_transparency<_Hash, _Kx>, int> = 0>
    size_type erase(_Kx&& _Keyval) noexcept(noexcept(_Erase(_Keyval))) /* strengthened */ {
        return _Erase(_Keyval);
    }
#endif // _HAS_CXX23

    void clear() noexcept {
        // TRANSITION, ABI:
        // LWG-2550 requires implementations to make clear() O(size()), independent of bucket_count().
        // Unfortunately our current data structure / ABI does not allow achieving this in the general case because:
        //   (1) Finding the bucket that goes with an element requires running the hash function
        //   (2) The hash function operator() may throw exceptions, and
        //   (3) clear() is a noexcept function.
        // We do comply with LWG-2550 if the hash function is noexcept, or if the container was empty.
        const auto _Oldsize = _List_Mypair()._Myval2._Mysize;
        if (_Oldsize == 0) {
            return;
        }

        if constexpr (_STD _Nothrow_hash<_Traits, key_type>) {
            // In testing, hash<size_t>{}(size_t{}) takes about 14 times as much time as assigning a pointer, or
            // ~7-8 times as much as clearing a bucket. Therefore, if we would need to assign over more than 8 times
            // as many buckets as elements, remove element-by-element.
            if (bucket_count() / 8 > _Oldsize) {
                const auto _Head = _List_Mypair()._Myval2._Myhead;
                _Unchecked_erase(_Head->_Next, _Head);
                return;
            }
        }

        // Bulk destroy items and reset buckets
        _List.clear();
        _STD fill(_Vec._Mypair._Myval2._Myfirst, _Vec._Mypair._Myval2._Mylast, _Unchecked_end());
    }

private:
    template <class _Keyty>
    _NODISCARD _Nodeptr _Find_first(const _Keyty& _Keyval, const size_t _Hashval) const {
        // find node pointer to first node matching _Keyval (with hash _Hashval) if it exists; otherwise, end
        const size_type _Bucket = _Hashval & _Mask;
        _Nodeptr _Where         = _Vec._Mypair._Myval2._Myfirst[_Bucket << 1]._Ptr;
        const _Nodeptr _End     = _List_Mypair()._Myval2._Myhead;
        if (_Where == _End) {
            return _End;
        }

        const _Nodeptr _Bucket_hi = _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1]._Ptr;
        for (;;) {
            if (!_Traitsobj(_Traits::_Kfn(_Where->_Myval), _Keyval)) {
                if constexpr (!_Traits::_Standard) {
                    if (_Traitsobj(_Keyval, _Traits::_Kfn(_Where->_Myval))) {
                        return _End;
                    }
                }

                return _Where;
            }

            if (_Where == _Bucket_hi) {
                return _End;
            }

            _Where = _Where->_Next;
        }
    }

    template <class _Keyty>
    _Nodeptr _Find(const _Keyty& _Keyval, const size_t _Hashval) const {
        if constexpr (_Traits::_Multi) {
            return _Find_first(_Keyval, _Hashval);
        } else {
            // use _Find_last for unique containers to avoid increase in code size of instantiating _Find_first
            auto _Target = _Find_last(_Keyval, _Hashval)._Duplicate;
            if (_Target) {
                return _Target;
            }

            return _List_Mypair()._Myval2._Myhead;
        }
    }

public:
    template <class _Keyty = void>
    _NODISCARD iterator find(typename _Traits::template _Deduce_key<_Keyty> _Keyval) {
        return _List._Make_iter(_Find(_Keyval, _Traitsobj(_Keyval)));
    }

    template <class _Keyty = void>
    _NODISCARD const_iterator find(typename _Traits::template _Deduce_key<_Keyty> _Keyval) const {
        return _List._Make_const_iter(_Find(_Keyval, _Traitsobj(_Keyval)));
    }

#if _HAS_CXX20
    template <class _Keyty = void>
    _NODISCARD bool contains(_Traits::template _Deduce_key<_Keyty> _Keyval) const {
        return static_cast<bool>(_Find_last(_Keyval, _Traitsobj(_Keyval))._Duplicate);
    }
#endif // _HAS_CXX20

    template <class _Keyty = void>
    _NODISCARD size_type count(typename _Traits::template _Deduce_key<_Keyty> _Keyval) const {
        const size_t _Hashval = _Traitsobj(_Keyval);
        if constexpr (_Multi) {
            return _Equal_range(_Keyval, _Hashval)._Distance;
        } else {
            return static_cast<bool>(_Find_last(_Keyval, _Hashval)._Duplicate);
        }
    }

    _DEPRECATE_STDEXT_HASH_LOWER_BOUND _NODISCARD iterator lower_bound(const key_type& _Keyval) {
        return _List._Make_iter(_Find(_Keyval, _Traitsobj(_Keyval)));
    }

    _DEPRECATE_STDEXT_HASH_LOWER_BOUND _NODISCARD const_iterator lower_bound(const key_type& _Keyval) const {
        return _List._Make_const_iter(_Find(_Keyval, _Traitsobj(_Keyval)));
    }

    _DEPRECATE_STDEXT_HASH_UPPER_BOUND _NODISCARD iterator upper_bound(const key_type& _Keyval) {
        auto _Target = _Find_last(_Keyval, _Traitsobj(_Keyval))._Duplicate;
        if (_Target) {
            _Target = _Target->_Next;
        } else {
            _Target = _List_Mypair()._Myval2._Myhead;
        }

        return _List._Make_iter(_Target);
    }

    _DEPRECATE_STDEXT_HASH_UPPER_BOUND _NODISCARD const_iterator upper_bound(const key_type& _Keyval) const {
        auto _Target = _Find_last(_Keyval, _Traitsobj(_Keyval))._Duplicate;
        if (_Target) {
            _Target = _Target->_Next;
        } else {
            _Target = _List_Mypair()._Myval2._Myhead;
        }

        return _List._Make_const_iter(_Target);
    }

private:
    struct _Equal_range_result {
        _Unchecked_const_iterator _First;
        _Unchecked_const_iterator _Last;
        size_type _Distance;
    };

    template <class _Keyty>
    _NODISCARD _Equal_range_result _Equal_range(const _Keyty& _Keyval, const size_t _Hashval) const
        noexcept(_STD _Nothrow_compare<_Traits, key_type, _Keyty>&& _STD _Nothrow_compare<_Traits, _Keyty, key_type>) {
        const size_type _Bucket              = _Hashval & _Mask;
        _Unchecked_const_iterator _Where     = _Vec._Mypair._Myval2._Myfirst[_Bucket << 1];
        const _Unchecked_const_iterator _End = _Unchecked_end();
        if (_Where == _End) {
            return {_End, _End, 0};
        }

        const _Unchecked_const_iterator _Bucket_hi = _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1];
        for (; _Traitsobj(_Traits::_Kfn(*_Where), _Keyval); ++_Where) {
            if (_Where == _Bucket_hi) {
                return {_End, _End, 0};
            }
        }

        if constexpr (!_Traits::_Standard) {
            if (_Traitsobj(_Keyval, _Traits::_Kfn(*_Where))) {
                return {_End, _End, 0};
            }
        }

        const _Unchecked_const_iterator _First = _Where;
        if constexpr (_Multi) {
            size_type _Distance = 0;
            for (;;) {
                ++_Distance;

                const bool _At_bucket_end = _Where == _Bucket_hi;
                ++_Where;
                if (_At_bucket_end) {
                    break;
                }

                if (_Traitsobj(_Keyval, _Traits::_Kfn(*_Where))) {
                    break;
                }
            }

            return {_First, _Where, _Distance};
        } else {
            ++_Where; // found the unique element
            return {_First, _Where, 1};
        }
    }

public:
    template <class _Keyty = void>
    _NODISCARD _STD pair<iterator, iterator> equal_range(typename _Traits::template _Deduce_key<_Keyty> _Keyval) {
        const auto _Result = _Equal_range(_Keyval, _Traitsobj(_Keyval));
        return {_List._Make_iter(_Result._First._Ptr), _List._Make_iter(_Result._Last._Ptr)};
    }

    template <class _Keyty = void>
    _NODISCARD _STD pair<const_iterator, const_iterator> equal_range(
        typename _Traits::template _Deduce_key<_Keyty> _Keyval) const {
        const auto _Result = _Equal_range(_Keyval, _Traitsobj(_Keyval));
        return {_List._Make_const_iter(_Result._First._Ptr), _List._Make_const_iter(_Result._Last._Ptr)};
    }

    void swap(_Hash& _Right) noexcept(noexcept(_Traitsobj.swap(_Right._Traitsobj))) /* strengthened */ {
        if (this != _STD addressof(_Right)) {
            _Traitsobj.swap(_Right._Traitsobj);
            _Pocs(_List_Getal(), _Right._List_Getal());
            _Pocs(_Vec._Mypair._Get_first(), _Right._Vec._Mypair._Get_first());
            _Swap_val(_Right);
        }
    }

#if _HAS_CXX17
    using node_type = typename _Traits::node_type;

    node_type extract(const const_iterator _Where) {
#if _ITERATOR_DEBUG_LEVEL == 2
        const auto _List_data = _STD addressof(_List_Mypair()._Myval2);
        _STL_VERIFY(_Where._Getcont() == _List_data, "extract mismatched container");
        _STL_VERIFY(_Where._Ptr != _List_data->_Myhead, "cannot extract end()");
#endif // _ITERATOR_DEBUG_LEVEL == 2

        return node_type::_Make(_Extract(_Where._Unwrapped()), _List_Getal());
    }

    node_type extract(const key_type& _Keyval) {
        const auto _Ptr = _Extract(_Keyval);
        if (!_Ptr) {
            return node_type{};
        }

        return node_type::_Make(_Ptr, _List_Getal());
    }

#if _HAS_CXX23
    template <class _Kx, class _Mytraits = _Traits,
        _STD enable_if_t<_Mytraits::template _Supports_transparency<_Hash, _Kx>, int> = 0>
    node_type extract(_Kx&& _Keyval) {
        const auto _Ptr = _Extract(_Keyval);
        if (!_Ptr) {
            return node_type{};
        }

        return node_type::_Make(_Ptr, _List_Getal());
    }
#endif // _HAS_CXX23

    iterator insert(const_iterator _Hint, node_type&& _Handle) {
        if (_Handle.empty()) {
            return end();
        }

#if _ITERATOR_DEBUG_LEVEL == 2
        _STL_VERIFY(_List.get_allocator() == _Handle._Getal(), "node handle allocator incompatible for insert");
#endif // _ITERATOR_DEBUG_LEVEL == 2

        const auto& _Keyval   = _Traits::_Kfn(_Handle._Getptr()->_Myval);
        const size_t _Hashval = _Traitsobj(_Keyval);
        auto _Target          = _Find_hint(_Hint._Ptr, _Keyval, _Hashval);
        if constexpr (!_Traits::_Multi) {
            if (_Target._Duplicate) {
                return _List._Make_iter(_Target._Duplicate);
            }
        }

        _Check_max_size();
        if (_Check_rehash_required_1()) {
            _Rehash_for_1();
            _Target = _Find_hint(_Hint._Ptr, _Keyval, _Hashval);
        }

        const auto _Released = _Handle._Release();
        _Destroy_in_place(_Released->_Next); // TRANSITION, ABI
        _Destroy_in_place(_Released->_Prev);
        return _List._Make_iter(_Insert_new_node_before(_Hashval, _Target._Insert_before, _Released));
    }

    template <class>
    friend class _Hash;

    template <class _Other_traits>
    void merge(_Hash<_Other_traits>& _That) { // transfer all nodes from _That into *this
        static_assert(_STD is_same_v<_Nodeptr, typename _Hash<_Other_traits>::_Nodeptr>,
            "merge() requires an argument with a compatible node type.");

        static_assert(_STD is_same_v<allocator_type, typename _Hash<_Other_traits>::allocator_type>,
            "merge() requires an argument with the same allocator type.");

        if constexpr (_STD is_same_v<_Hash, _Hash<_Other_traits>>) {
            if (this == _STD addressof(_That)) {
                return;
            }
        }

#if _ITERATOR_DEBUG_LEVEL == 2
        if constexpr (!_Alnode_traits::is_always_equal::value) {
            _STL_VERIFY(_List_Getal() == _That._List_Getal(), "allocator incompatible for merge");
        }
#endif // _ITERATOR_DEBUG_LEVEL == 2

        auto _First      = _That._Unchecked_begin();
        const auto _Last = _That._Unchecked_end();
        while (_First != _Last) {
            const auto _Candidate = _First._Ptr;
            ++_First;
            const auto& _Keyval   = _Traits::_Kfn(_Candidate->_Myval);
            const size_t _Hashval = _Traitsobj(_Keyval);
            auto _Target          = _Find_last(_Keyval, _Hashval);
            if constexpr (!_Traits::_Multi) {
                if (_Target._Duplicate) {
                    continue;
                }
            }

            _Check_max_size();
            if (_Check_rehash_required_1()) {
                _Rehash_for_1();
                _Target = _Find_last(_Keyval, _Hashval);
            }

            // nothrow hereafter this iteration
            const auto _Source_bucket = _Hashval & _That._Mask;
            _That._Erase_bucket(_Candidate, _Source_bucket);
            _Candidate->_Prev->_Next = _Candidate->_Next;
            _Candidate->_Next->_Prev = _Candidate->_Prev;
            --_That._List_Mypair()._Myval2._Mysize;
            _Destroy_in_place(_Candidate->_Next); // TRANSITION, ABI
            _Destroy_in_place(_Candidate->_Prev);
#if _ITERATOR_DEBUG_LEVEL == 2
            _List_Mypair()._Myval2._Adopt_unique(_That._List_Mypair()._Myval2, _Candidate);
#endif // _ITERATOR_DEBUG_LEVEL == 2
            (void) _Insert_new_node_before(_Hashval, _Target._Insert_before, _Candidate);
        }
    }

    template <class _Other_traits>
    void merge(_Hash<_Other_traits>&& _That) { // transfer all nodes from _That into *this
        static_assert(_STD is_same_v<_Nodeptr, typename _Hash<_Other_traits>::_Nodeptr>,
            "merge() requires an argument with a compatible node type.");

        static_assert(_STD is_same_v<allocator_type, typename _Hash<_Other_traits>::allocator_type>,
            "merge() requires an argument with the same allocator type.");

        merge(_That);
    }

protected:
    _Nodeptr _Extract(const _Unchecked_const_iterator _Where) {
        const size_type _Bucket = bucket(_Traits::_Kfn(*_Where));
        _Erase_bucket(_Where._Ptr, _Bucket);
        return _List_Mypair()._Myval2._Unlinknode(_Where._Ptr);
    }

    template <class _Kx>
    _Nodeptr _Extract(const _Kx& _Keyval) {
        const size_t _Hashval = _Traitsobj(_Keyval);
        _Nodeptr _Target;
        if constexpr (_Traits::_Multi) {
            _Target = _Find_first(_Keyval, _Hashval);
            if (_Target == _List_Mypair()._Myval2._Myhead) {
                return _Nodeptr{};
            }
        } else {
            _Target = _Find_last(_Keyval, _Hashval)._Duplicate;
            if (_Target == nullptr) {
                return _Nodeptr{};
            }
        }

        _Erase_bucket(_Target, _Hashval & _Mask);
        return _List_Mypair()._Myval2._Unlinknode(_Target);
    }

public:
    _STD conditional_t<_Traits::_Multi, iterator, _STD _Insert_return_type<iterator, node_type>> insert(node_type&& _Handle) {
        // insert the node (if any) held in _Handle
        if (_Handle.empty()) {
            if constexpr (_Traits::_Multi) {
                return end();
            } else {
                return {end(), false, _STD move(_Handle)};
            }
        }

#if _ITERATOR_DEBUG_LEVEL == 2
        _STL_VERIFY(_List.get_allocator() == _Handle._Getal(), "node handle allocator incompatible for insert");
#endif // _ITERATOR_DEBUG_LEVEL == 2

        const auto& _Keyval   = _Traits::_Kfn(_Handle._Getptr()->_Myval);
        const size_t _Hashval = _Traitsobj(_Keyval);
        auto _Target          = _Find_last(_Keyval, _Hashval);
        if constexpr (!_Traits::_Multi) {
            if (_Target._Duplicate) {
                return {_List._Make_iter(_Target._Duplicate), false, _STD move(_Handle)};
            }
        }

        _Check_max_size();
        if (_Check_rehash_required_1()) {
            _Rehash_for_1();
            _Target = _Find_last(_Keyval, _Hashval);
        }

        const auto _Released = _Handle._Release();
        _Destroy_in_place(_Released->_Next); // TRANSITION, ABI
        _Destroy_in_place(_Released->_Prev);
        const auto _Newnode = _Insert_new_node_before(_Hashval, _Target._Insert_before, _Released);
        if constexpr (_Traits::_Multi) {
            return _List._Make_iter(_Newnode);
        } else {
            return {_List._Make_iter(_Newnode), true, node_type{}};
        }
    }
#endif // _HAS_CXX17

protected:
    template <class _Keyty>
    _NODISCARD _STD _Hash_find_last_result<_Nodeptr> _Find_last(const _Keyty& _Keyval, const size_t _Hashval) const {
        // find the insertion point for _Keyval and whether an element identical to _Keyval is already in the container
        const size_type _Bucket = _Hashval & _Mask;
        _Nodeptr _Where         = _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1]._Ptr;
        const _Nodeptr _End     = _List_Mypair()._Myval2._Myhead;
        if (_Where == _End) {
            return {_End, _Nodeptr{}};
        }

        const _Nodeptr _Bucket_lo = _Vec._Mypair._Myval2._Myfirst[_Bucket << 1]._Ptr;
        for (;;) {
            // Search backwards to maintain sorted [_Bucket_lo, _Bucket_hi] when !_Standard
            if (!_Traitsobj(_Keyval, _Traits::_Kfn(_Where->_Myval))) {
                if constexpr (!_Traits::_Standard) {
                    if (_Traitsobj(_Traits::_Kfn(_Where->_Myval), _Keyval)) {
                        return {_Where->_Next, _Nodeptr{}};
                    }
                }

                return {_Where->_Next, _Where};
            }

            if (_Where == _Bucket_lo) {
                return {_Where, _Nodeptr{}};
            }

            _Where = _Where->_Prev;
        }
    }

    template <class _Keyty>
    _NODISCARD _STD _Hash_find_last_result<_Nodeptr> _Find_hint(
        const _Nodeptr _Hint, const _Keyty& _Keyval, const size_t _Hashval) const {
        // if _Hint points to an element equivalent to _Keyval, returns _Hint; otherwise,
        // returns _Find_last(_Keyval, _Hashval)
        if (_Hint != _List_Mypair()._Myval2._Myhead && !_Traitsobj(_Traits::_Kfn(_Hint->_Myval), _Keyval)) {
            if constexpr (!_Traits::_Standard) {
                if (_Traitsobj(_Keyval, _Traits::_Kfn(_Hint->_Myval))) {
                    return _Find_last(_Keyval, _Hashval);
                }
            }

            return {_Hint->_Next, _Hint};
        }

        return _Find_last(_Keyval, _Hashval);
    }

    _Nodeptr _Insert_new_node_before(
        const size_t _Hashval, const _Nodeptr _Insert_before, const _Nodeptr _Newnode) noexcept {
        const _Nodeptr _Insert_after = _Insert_before->_Prev;
        ++_List_Mypair()._Myval2._Mysize;
        _Construct_in_place(_Newnode->_Next, _Insert_before);
        _Construct_in_place(_Newnode->_Prev, _Insert_after);
        _Insert_after->_Next  = _Newnode;
        _Insert_before->_Prev = _Newnode;

        const auto _Head                = _List_Mypair()._Myval2._Myhead;
        const auto _Bucket_array        = _Vec._Mypair._Myval2._Myfirst;
        const size_type _Bucket         = _Hashval & _Mask;
        _Unchecked_iterator& _Bucket_lo = _Bucket_array[_Bucket << 1];
        _Unchecked_iterator& _Bucket_hi = _Bucket_array[(_Bucket << 1) + 1];
        if (_Bucket_lo._Ptr == _Head) {
            // bucket is empty, set both
            _Bucket_lo._Ptr = _Newnode;
            _Bucket_hi._Ptr = _Newnode;
        } else if (_Bucket_lo._Ptr == _Insert_before) {
            // new node is the lowest element in the bucket
            _Bucket_lo._Ptr = _Newnode;
        } else if (_Bucket_hi._Ptr == _Insert_after) {
            // new node is the highest element in the bucket
            _Bucket_hi._Ptr = _Newnode;
        }

#ifdef _ENABLE_STL_INTERNAL_CHECK
        _Stl_internal_check_container_invariants();
#endif // _ENABLE_STL_INTERNAL_CHECK
        return _Newnode;
    }

    void _Check_max_size() const {
        const size_type _Oldsize = _List_Mypair()._Myval2._Mysize;
        if (_Oldsize == _List.max_size()) {
            _STD _Xlength_error("unordered_map/set too long");
        }
    }

    bool _Check_rehash_required_1() const noexcept {
        const size_type _Oldsize = _List_Mypair()._Myval2._Mysize;
        const auto _Newsize      = _Oldsize + 1;
        return max_load_factor() < static_cast<float>(_Newsize) / static_cast<float>(bucket_count());
    }

    void _Rehash_for_1() {
        const auto _Oldsize = _List_Mypair()._Myval2._Mysize;
        const auto _Newsize = _Oldsize + 1;
        _Forced_rehash(_Desired_grow_bucket_count(_Newsize));
    }

    void _Erase_bucket(_Nodeptr _Plist, size_type _Bucket) noexcept {
        // remove the node _Plist from its bucket
        _Nodeptr& _Bucket_lo = _Vec._Mypair._Myval2._Myfirst[_Bucket << 1]._Ptr;
        _Nodeptr& _Bucket_hi = _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1]._Ptr;
        if (_Bucket_hi == _Plist) {
            if (_Bucket_lo == _Plist) { // make bucket empty
                const auto _End = _List_Mypair()._Myval2._Myhead;

                _Bucket_lo = _End;
                _Bucket_hi = _End;
            } else {
                _Bucket_hi = _Plist->_Prev; // move end back one element
            }
        } else if (_Bucket_lo == _Plist) {
            _Bucket_lo = _Plist->_Next; // move beginning up one element
        }
    }

    _NODISCARD size_type _Min_load_factor_buckets(const size_type _For_size) const noexcept {
        // returns the minimum number of buckets necessary for the elements in _List
        return static_cast<size_type>(_CSTD ceilf(static_cast<float>(_For_size) / max_load_factor()));
    }

    _NODISCARD size_type _Desired_grow_bucket_count(const size_type _For_size) const noexcept {
        const size_type _Old_buckets = bucket_count();
        const size_type _Req_buckets = (_STD max)(_Min_buckets, _Min_load_factor_buckets(_For_size));
        if (_Old_buckets >= _Req_buckets) {
            // we already have enough buckets so there's no need to change the count
            return _Old_buckets;
        }

        if (_Old_buckets < 512 && _Old_buckets * 8 >= _Req_buckets) {
            // if we are changing the bucket count and have less than 512 buckets, use 8x more buckets
            return _Old_buckets * 8;
        }

        // power of 2 invariant means this will result in at least 2*_Old_buckets after round up in _Forced_rehash
        return _Req_buckets;
    }

    void _Reinsert_with_invalid_vec() { // insert elements in [begin(), end()), distrusting existing _Vec elements
        _Forced_rehash(_Desired_grow_bucket_count(_List.size()));
    }

    void _Forced_rehash(size_type _Buckets) {
        // Force rehash of elements in _List, distrusting existing bucket assignments in _Vec.
        // Assumes _Buckets is greater than _Min_buckets, and that changing to that many buckets doesn't violate
        // load_factor() <= max_load_factor().

        // Don't violate power of 2, fits in half the bucket vector invariant:
        // (we assume because vector must use single allocations; as a result, its max_size fits in a size_t)
        const unsigned long _Max_storage_buckets_log2 = _STD _Floor_of_log_2(static_cast<size_t>(_Vec.max_size() >> 1));
        const auto _Max_storage_buckets               = static_cast<size_type>(1) << _Max_storage_buckets_log2;
        if (_Buckets > _Max_storage_buckets) {
            _STD _Xlength_error("invalid hash bucket count");
        }

        // The above test also means that we won't perform a forbidden full shift when restoring the power of
        // 2 invariant
        // this round up to power of 2 in addition to the _Buckets > _Maxidx above means
        // we'll at least double in size (the next power of 2 above _Maxidx)
        _Buckets                       = static_cast<size_type>(1) << _STD _Ceiling_of_log_2(static_cast<size_t>(_Buckets));
        const _Unchecked_iterator _End = _Unchecked_end();

        _Vec._Assign_grow(_Buckets << 1, _End);
        _Mask   = _Buckets - 1;
        _Maxidx = _Buckets;

        _Clear_guard _Guard{this};

        _Unchecked_iterator _Inserted = _Unchecked_begin();

        // Remember the next _Inserted value as splices will change _Inserted's position arbitrarily.
        for (_Unchecked_iterator _Next_inserted = _Inserted; _Inserted != _End; _Inserted = _Next_inserted) {
            ++_Next_inserted;

            auto& _Inserted_key     = _Traits::_Kfn(*_Inserted);
            const size_type _Bucket = bucket(_Inserted_key);

            // _Bucket_lo and _Bucket_hi are the *inclusive* range of elements in the bucket, or _Unchecked_end() if
            // the bucket is empty; if !_Standard then [_Bucket_lo, _Bucket_hi] is a sorted range.
            _Unchecked_iterator& _Bucket_lo = _Vec._Mypair._Myval2._Myfirst[_Bucket << 1];
            _Unchecked_iterator& _Bucket_hi = _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1];

            if (_Bucket_lo == _End) {
                // The bucket was empty, set it to the inserted element.
                _Bucket_lo = _Inserted;
                _Bucket_hi = _Inserted;
                continue;
            }

            // Search the bucket for the insertion location and move element if necessary.
            _Unchecked_const_iterator _Insert_before = _Bucket_hi;
            if (!_Traitsobj(_Inserted_key, _Traits::_Kfn(*_Insert_before))) {
                // The inserted element belongs at the end of the bucket; splice it there and set _Bucket_hi to the
                // new bucket inclusive end.
                ++_Insert_before;
                if (_Insert_before != _Inserted) { // avoid splice on element already in position
                    _MylistImpl::_Scary_val::_Unchecked_splice(_Insert_before._Ptr, _Inserted._Ptr, _Next_inserted._Ptr);
                }

                _Bucket_hi = _Inserted;
                continue;
            }

            // The insertion point isn't *_Bucket_hi, so search [_Bucket_lo, _Bucket_hi) for insertion point; we
            // go backwards to maintain sortedness when !_Standard.
            for (;;) {
                if (_Bucket_lo == _Insert_before) {
                    // There are no equivalent keys in the bucket, so insert it at the beginning.
                    // Element can't be already in position here because:
                    // * (for !_Standard) _Inserted_key < *_Insert_before or
                    // * (for _Standard) _Inserted_key != *_Insert_before
                    _MylistImpl::_Scary_val::_Unchecked_splice(_Insert_before._Ptr, _Inserted._Ptr, _Next_inserted._Ptr);
                    _Bucket_lo = _Inserted;
                    break;
                }

                if (!_Traitsobj(_Inserted_key, _Traits::_Kfn(*--_Insert_before))) {
                    // Found insertion point, move the element here, bucket bounds are already okay.
                    ++_Insert_before;
                    // Element can't be already in position here because all elements we're inserting are after all
                    // the elements already in buckets, and *_Insert_before isn't the highest element in the bucket.
                    _MylistImpl::_Scary_val::_Unchecked_splice(_Insert_before._Ptr, _Inserted._Ptr, _Next_inserted._Ptr);
                    break;
                }
            }
        }

        _Guard._Target = nullptr;

#ifdef _ENABLE_STL_INTERNAL_CHECK
        _Stl_internal_check_container_invariants();
#endif // _ENABLE_STL_INTERNAL_CHECK
    }

    float& _Max_bucket_size() noexcept {
        return _Traitsobj._Get_max_bucket_size();
    }

    const float& _Max_bucket_size() const noexcept {
        return _Traitsobj._Get_max_bucket_size();
    }

    _Alnode& _Getal() noexcept {
        return _List_Getal();
    }

    const _Alnode& _Getal() const noexcept {
        return _List_Getal();
    }

    struct _Multi_equal_check_result {
        bool _Equal_possible = false;
        _Unchecked_const_iterator _Subsequent_first{}; // only useful if _Equal_possible
    };

    _NODISCARD _Multi_equal_check_result _Multi_equal_check_equal_range(
        const _Hash& _Right, _Unchecked_const_iterator _First1) const {
        // check that an equal_range of elements starting with *_First1 are a permutation of the corresponding
        // equal_range of elements in _Right
        auto& _Keyval = _Traits::_Kfn(*_First1);
        // find the start of the matching run in the other container
        const size_t _Hashval   = _Right._Traitsobj(_Keyval);
        const size_type _Bucket = _Hashval & _Right._Mask;
        auto _First2            = _Right._Vec._Mypair._Myval2._Myfirst[_Bucket << 1];
        if (_First2 == _Right._Unchecked_end()) {
            // no matching bucket, therefore no matching run
            return {};
        }

        const auto _Bucket_hi = _Right._Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1];
        for (; _Right._Traitsobj(_Traits::_Kfn(*_First2), _Keyval); ++_First2) {
            // find first matching element in _Right
            if (_First2 == _Bucket_hi) {
                return {};
            }
        }

        _Unchecked_const_iterator _Left_stop_at;
        if constexpr (_Traits::_Standard) {
            _Left_stop_at = _Unchecked_end();
        } else {
            // check the first elements for equivalence when !_Standard
            if (_Right._Traitsobj(_Keyval, _Traits::_Kfn(*_First2))) {
                return {};
            }

            const size_t _LHashval   = _Traitsobj(_Keyval);
            const size_type _LBucket = _LHashval & _Mask;
            const auto _LBucket_hi   = _Vec._Mypair._Myval2._Myfirst[(_LBucket << 1) + 1];
            _Left_stop_at            = _LBucket_hi;
            ++_Left_stop_at;
        }

        // trim matching prefixes
        while (*_First1 == *_First2) {
            // the right equal_range ends at the end of the bucket or on the first nonequal element
            bool _Right_range_end = _First2 == _Bucket_hi;
            ++_First2;
            if (!_Right_range_end) {
                _Right_range_end = _Right._Traitsobj(_Keyval, _Traits::_Kfn(*_First2));
            }

            // the left equal_range ends at the end of the container or on the first nonequal element
            ++_First1;
            const bool _Left_range_end = _First1 == _Left_stop_at || _Traitsobj(_Keyval, _Traits::_Kfn(*_First1));

            if (_Left_range_end && _Right_range_end) {
                // the equal_ranges were completely equal
                return {true, _First1};
            }

            if (_Left_range_end || _Right_range_end) {
                // one equal_range is a prefix of the other; not equal
                return {};
            }
        }

        // found a mismatched element, find the end of the equal_ranges and dispatch to _Check_match_counts
        auto _Last1 = _First1;
        auto _Last2 = _First2;
        for (;;) {
            bool _Right_range_end = _Last2 == _Bucket_hi;
            ++_Last2;
            if (!_Right_range_end) {
                _Right_range_end = _Right._Traitsobj(_Keyval, _Traits::_Kfn(*_Last2));
            }

            ++_Last1;
            const bool _Left_range_end = _Last1 == _Left_stop_at || _Traitsobj(_Keyval, _Traits::_Kfn(*_Last1));

            if (_Left_range_end && _Right_range_end) {
                // equal_ranges had the same length, check for permutation
                return {_STD _Check_match_counts(_First1, _Last1, _First2, _Last2, _STD equal_to<>{}), _Last1};
            }

            if (_Left_range_end || _Right_range_end) {
                // different number of elements in the range, not a permutation
                return {};
            }
        }
    }

    template <bool _Multi2 = _Traits::_Multi, _STD enable_if_t<_Multi2, int> = 0>
    _NODISCARD bool _Multi_equal(const _Hash& _Right) const {
        static_assert(_Traits::_Multi, "This function only works with multi containers");
        _STL_INTERNAL_CHECK(this->size() == _Right.size());
        const auto _Last1 = _Unchecked_end();
        auto _First1      = _Unchecked_begin();
        while (_First1 != _Last1) {
            const auto _Result = _Multi_equal_check_equal_range(_Right, _First1);
            if (!_Result._Equal_possible) {
                return false;
            }

            _First1 = _Result._Subsequent_first;
        }

        return true;
    }

#ifdef _ENABLE_STL_INTERNAL_CHECK
public:
    void _Stl_internal_check_container_invariants() const noexcept {
        const size_type _Vecsize = _Vec.size();
        _STL_INTERNAL_CHECK(_Vec._Mypair._Myval2._Mylast == _Vec._Mypair._Myval2._Myend);
        _STL_INTERNAL_CHECK(_Vecsize >= _Min_buckets * 2);
        _STL_INTERNAL_CHECK(_Maxidx == (_Vecsize >> 1));
        _STL_INTERNAL_CHECK(_Maxidx - 1 == _Mask);
        _STL_INTERNAL_CHECK(_Maxidx >= _Min_load_factor_buckets(_List.size()));
        // asserts that bucket count is a power of 2:
        _STL_INTERNAL_CHECK((static_cast<size_type>(1) << _STD _Floor_of_log_2(_Vecsize)) == _Vecsize);
        _STL_INTERNAL_CHECK(load_factor() <= max_load_factor());
        // In the test that counts number of allocator copies, avoid an extra rebind that would incorrectly count as
        // a copy; otherwise, allow allocators that support only homogeneous compare.
#ifdef _USE_HETEROGENEOUS_ALLOCATOR_COMPARE_IN_INTERNAL_CHECK
        _STL_INTERNAL_CHECK(_List_Getal() == _Vec._Mypair._Get_first());
#else
        _STL_INTERNAL_CHECK(static_cast<_Aliter>(_List_Getal()) == _Vec._Mypair._Get_first());
#endif
#ifdef _STL_INTERNAL_CHECK_EXHAUSTIVE
        size_type _Elements = 0;
        const auto _End     = _Unchecked_end();
        for (size_type _Bucket = 0; _Bucket < _Maxidx; ++_Bucket) {
            _Unchecked_const_iterator _Where           = _Vec._Mypair._Myval2._Myfirst[_Bucket << 1];
            const _Unchecked_const_iterator _Bucket_hi = _Vec._Mypair._Myval2._Myfirst[(_Bucket << 1) + 1];
            if (_Where != _End) {
                // check that the bucket is sorted for legacy hash_meow:
                if constexpr (!_Traits::_Standard) {
                    if (_Where != _Bucket_hi) {
                        auto _SFirst = _Where;
                        auto _SNext  = _Where;
                        for (;;) {
                            ++_SNext;
                            if constexpr (_Traits::_Multi) {
                                _STL_INTERNAL_CHECK(!_Traitsobj(_Traits::_Kfn(*_SNext), _Traits::_Kfn(*_SFirst)));
                            } else {
                                _STL_INTERNAL_CHECK(_Traitsobj(_Traits::_Kfn(*_SFirst), _Traits::_Kfn(*_SNext)));
                            }

                            if (_SNext == _Bucket_hi) {
                                break;
                            }

                            _SFirst = _SNext;
                        }
                    }
                }
                // check that all the elements in the bucket belong in the bucket:
                for (;;) {
                    ++_Elements;
                    _STL_INTERNAL_CHECK(bucket(_Traits::_Kfn(*_Where)) == _Bucket);
                    if (_Where == _Bucket_hi) {
                        break;
                    }

                    ++_Where;
                }
            }
        }

        _STL_INTERNAL_CHECK(_List.size() == _Elements);
#endif // _STL_INTERNAL_CHECK_EXHAUSTIVE
    }

protected:
#endif // _ENABLE_STL_INTERNAL_CHECK

    _Mylist _List; // list of elements, must initialize before _Vec
    _STD _Hash_vec<_Aliter> _Vec; // "vector" of list iterators for buckets:
                             // each bucket is 2 iterators denoting the closed range of elements in the bucket,
                             // or both iterators set to _Unchecked_end() if the bucket is empty.
    size_type _Mask; // the key mask
    size_type _Maxidx; // current maximum key value, must be a power of 2
    _Traits _Traitsobj; // traits to customize behavior
};

#if _HAS_CXX17
// For constraining deduction guides (N4950 [unord.req.general]/243.3)
template <class _Hasher>
using _Is_hasher = _STD negation<_STD disjunction<_STD is_integral<_Hasher>, _STD _Is_allocator<_Hasher>>>;
#endif // _HAS_CXX17

/*_EXPORT_STD*/ /* TRANSITION, VSO-1538698 */ template <class _Traits>
_NODISCARD bool _Hash_equal(const _Hash<_Traits>& _Left, const _Hash<_Traits>& _Right) {
    if (_Left.size() != _Right.size()) {
        return false;
    }

    if constexpr (_Traits::_Multi) {
        return _Left._Multi_equal(_Right);
    } else {
        for (const auto& _LVal : _Left) {
            // look for element with equivalent key
            const auto& _Keyval = _Traits::_Kfn(_LVal);
            const auto _Next2   = _Right._Find_last(_Keyval, _Right._Traitsobj(_Keyval))._Duplicate;
            if (!(static_cast<bool>(_Next2) && _Traits::_Nonkfn(_LVal) == _Traits::_Nonkfn(_Next2->_Myval))) {
                return false;
            }
        }
    }

    return true;
}
_STD_END

#pragma pop_macro("new")
_STL_RESTORE_CLANG_WARNINGS
#pragma warning(pop)
#pragma pack(pop)
#endif // _STL_COMPILER_PREPROCESSOR
#endif // _GEODE_XHASH_
