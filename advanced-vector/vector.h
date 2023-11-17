    #pragma once
    #include <cassert>
    #include <cstdlib>
    #include <new>
    #include <utility>
    #include <memory>
    #include <algorithm>
    #include <stdexcept>

    template <typename T>
    class RawMemory {
    public:
        RawMemory() = default;

        explicit RawMemory(size_t capacity)
                : buffer_(Allocate(capacity))
                , capacity_(capacity) {
        }

        RawMemory(const RawMemory&) = delete;

        RawMemory& operator=(const RawMemory& rhs) = delete;

        RawMemory(RawMemory&& other) noexcept {
            buffer_ = other.buffer_;
            capacity_ = other.capacity_;
            other.buffer_ = nullptr;
            other.capacity_ = 0;
        }

        RawMemory& operator=(RawMemory&& rhs) noexcept {
            if (this != &rhs) {
                buffer_.~RawMemory();
                capacity_ = 0;
                Swap(rhs);
            }
            return *this;
        }

        ~RawMemory() {
            Deallocate(buffer_);
        }

        T* operator+(size_t offset) noexcept {
            // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
            assert(offset <= capacity_);
            return buffer_ + offset;
        }

        const T* operator+(size_t offset) const noexcept {
            return const_cast<RawMemory&>(*this) + offset;
        }

        const T& operator[](size_t index) const noexcept {
            return const_cast<RawMemory&>(*this)[index];
        }

        T& operator[](size_t index) noexcept {
            assert(index < capacity_);
            return buffer_[index];
        }

        void Swap(RawMemory& other) noexcept {
            std::swap(buffer_, other.buffer_);
            std::swap(capacity_, other.capacity_);
        }

        const T* GetAddress() const noexcept {
            return buffer_;
        }

        T* GetAddress() noexcept {
            return buffer_;
        }

        size_t Capacity() const {
            return capacity_;
        }

    private:
        // Выделяет сырую память под n элементов и возвращает указатель на неё
        static T* Allocate(size_t n) {
            return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
        }

        // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
        static void Deallocate(T* buf) noexcept {
            operator delete(buf);
        }

        T* buffer_ = nullptr;
        size_t capacity_ = 0;
    };

    template <typename T>
    class Vector {
    public:
    // Сигнатура методов:
        using iterator = T*;
        using const_iterator = const T*;

        iterator begin() noexcept;
        iterator end() noexcept;
        const_iterator begin() const noexcept;
        const_iterator end() const noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        template <typename... Args>
        iterator Emplace(const_iterator pos, Args&&... args);
        iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/;
        iterator Insert(const_iterator pos, const T& value);
        iterator Insert(const_iterator pos, T&& value);

        Vector() = default;

        explicit Vector(size_t size);

        Vector(const Vector& other);

        Vector& operator=(const Vector& rhs);
        Vector(Vector&& other) noexcept;
        Vector& operator=(Vector&& rhs) noexcept;

        ~Vector();

        size_t Size() const noexcept;
        size_t Capacity() const noexcept;

        const T& operator[](size_t index) const noexcept;
        T& operator[](size_t index) noexcept;

        void Reserve(size_t new_capacity);
        void Swap(Vector& other) noexcept;
        void Resize(size_t new_size);

        template <typename Type>
        void PushBack(Type&& value);
        void PopBack();

        template <typename... Args>
        T& EmplaceBack(Args&&... args);

    private:
        RawMemory<T> data_;
        size_t size_ = 0;

        void CopyOrMoveElements(T* source, T* destination, size_t count);

        template<typename ...Args>
        typename Vector<T>::iterator EmplaceWithReallocation(const_iterator pos, Args && ...args);

        template<typename ...Args>
        typename Vector<T>::iterator EmplaceWithoutReallocation(const_iterator pos, Args && ...args);
    };

    template <typename T>
    typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    template <typename T>
    typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    template<typename T>
    template<typename ...Args>
    typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args && ...args) {
        assert(pos >= begin() && pos <= end());
        iterator result;
        if (size_ == Capacity()) {
            result = EmplaceWithReallocation(pos, std::forward<Args>(args)...);
        }
        else {
            result = EmplaceWithoutReallocation(pos, std::forward<Args>(args)...);
        }
        ++size_;
        return result;
    }

    template<typename T>
    typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos) {
        assert(pos >= begin() && pos < end());

        iterator erase_pos = begin() + (pos - cbegin());
        std::move(erase_pos + 1, end(), erase_pos);
        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;

        return erase_pos;
    }

    template <typename T>
    typename Vector<T>::iterator Vector<T>::begin() noexcept {
        return data_.GetAddress();
    }

    template <typename T>
    typename Vector<T>::const_iterator Vector<T>::begin() const noexcept {
        return data_.GetAddress();
    }

    template <typename T>
    typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
        return data_.GetAddress();
    }

    template <typename T>
    typename Vector<T>::iterator Vector<T>::end() noexcept {
        return data_.GetAddress() + size_;
    }

    template <typename T>
    typename Vector<T>::const_iterator Vector<T>::end() const noexcept {
        return data_.GetAddress() + size_;
    }

    template <typename T>
    typename Vector<T>::const_iterator Vector<T>::cend() const noexcept {
        return data_.GetAddress() + size_;
    }
    // исправил недочеты и ошибки
    template <typename T>
    Vector<T>::Vector(size_t size)
            : data_(size)
            , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    template <typename T>
    Vector<T>::Vector(const Vector& other)
            : data_(other.size_)
            , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    template<typename T>
    Vector<T>& Vector<T>::operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                /* Применить copy-and-swap */
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                /* Скопировать элементы из rhs, создав при необходимости новые
                   или удалив существующие */
                size_t copy_size = std::min(size_, rhs.size_);
                std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + copy_size, data_.GetAddress());
                if (size_ > rhs.size_) {
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                else if (rhs.size_ > size_) {
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    template<typename T>
    Vector<T>::Vector(Vector&& other) noexcept {
        Swap(other);
    }

    template<typename T>
    Vector<T>& Vector<T>::operator=(Vector&& rhs) noexcept {
    if (this != &rhs) {
    Swap(rhs);
    }
    return *this;
    }

    template<typename T>
    Vector<T>::~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    template<typename T>
    size_t Vector<T>::Size() const noexcept {
        return size_;
    }

    template<typename T>
    size_t Vector<T>::Capacity() const noexcept {
        return data_.Capacity();
    }

    template<typename T>
    const T& Vector<T>::operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    template<typename T>
    T& Vector<T>::operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    template<typename T>
    void Vector<T>::Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        CopyOrMoveElements(data_.GetAddress(), new_data.GetAddress(), size_);
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    template<typename T>
    void Vector<T>::Swap(Vector& other) noexcept {
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
    }

    template<typename T>
    void Vector<T>::Resize(size_t new_size) {
        if (new_size > size_) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        else {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        size_ = new_size;
    }
    template<typename T>
    template <typename Type>
    void Vector<T>::PushBack(Type&& value) {
        EmplaceBack(std::forward<Type>(value));
    }
    template<typename T>
    void Vector<T>::PopBack() /* noexcept */ {
        if (size_ > 0) {
            std::destroy_at(data_.GetAddress() + size_ - 1);
            --size_;
        }
    }
    template<typename T>
    template <typename... Args>
    T& Vector<T>::EmplaceBack(Args&&... args) {
        T* result = nullptr;
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            result = new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
            CopyOrMoveElements(data_.GetAddress(), new_data.GetAddress(), size_);
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            result = new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *result;
    }

    template<typename T>
    void Vector<T>::CopyOrMoveElements(T* source, T* destination, size_t count) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(source, count, destination);
        }
        else {
            std::uninitialized_copy_n(source, count, destination);
        }
    }
    template<typename T>
    template<typename ...Args>
    typename Vector<T>::iterator Vector<T>::EmplaceWithReallocation(const_iterator pos, Args && ...args) {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        iterator result = new (new_data.GetAddress() + (pos - begin())) T(std::forward<Args>(args)...);
        CopyOrMoveElements(begin(), new_data.GetAddress(), pos - begin());
        CopyOrMoveElements(begin() + (pos - begin()), new_data.GetAddress() + (pos - begin()) + 1, size_ - (pos - begin()));
        std::destroy_n(begin(), size_);
        data_.Swap(new_data);
        return result;
    }

    template<typename T>
    template<typename ...Args>
    typename Vector<T>::iterator Vector<T>::EmplaceWithoutReallocation(const_iterator pos, Args && ...args) {
        if (size_ != 0) {
            new (data_ + size_) T(std::move(*(end() - 1)));
            try {
                std::move_backward(begin() + (pos - begin()), end(), end() + 1);
            }
            catch (...) {
                std::destroy_n(end(), 1);
                throw;
            }
            std::destroy_at(begin() + (pos - begin()));
        }
        iterator result = new (data_ + (pos - begin())) T(std::forward<Args>(args)...);
        return result;
    }
