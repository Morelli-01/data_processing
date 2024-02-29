//
// Created by nicola on 29/02/24.
//

#include "stdlib.h"

typedef struct vector {
    int *data_;
    int capacity_;
    int size_;
} vector;

void construnctor(vector *this) {
    this->capacity_ = 10;
    this->size_ = 0;
    this->data_ = malloc(this->capacity_ * sizeof(int));
}

void destructor(vector *this) {
    free(this->data_);
}

void push_back(vector *this, int num) {
    if (this->size_ == this->capacity_) {
        this->capacity_ *= 2;
        this->data_ = (int *) realloc(this->data_, this->capacity_ * sizeof(int));

    }
    this->data_[this->size_] = num;
    this->size_++;
}

int main(int argc, char **argv) {
    return 0;
}