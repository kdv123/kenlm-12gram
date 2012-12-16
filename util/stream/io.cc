#include "util/stream/io.hh"

#include "util/file.hh"
#include "util/stream/chain.hh"

#include <cstddef>

namespace util {
namespace stream {

ReadSizeException::ReadSizeException() throw() {}
ReadSizeException::~ReadSizeException() throw() {}

void Read::Run(const ChainPosition &position) {
  const std::size_t block_size = position.GetChain().BlockSize();
  const std::size_t entry_size = position.GetChain().EntrySize();
  for (Link link(position); link; ++link) {
    std::size_t got = util::ReadOrEOF(file_, link->Get(), block_size);
    UTIL_THROW_IF(got % entry_size, ReadSizeException, "File ended with " << got << " bytes, not a multiple of " << entry_size << "."); 
    if (got == 0) {
      link.Poison();
      return;
    } else {
      link->SetValidSize(got);
    }
  }
}

void PRead::Run(const ChainPosition &position) {
  uint64_t size = SizeOrThrow(file_);
  UTIL_THROW_IF(size % static_cast<uint64_t>(position.GetChain().EntrySize()), ReadSizeException, "File size " << file_ << " size is " << size << " not a multiple of " << position.GetChain().EntrySize());
  std::size_t block_size = position.GetChain().BlockSize();
  Link link(position);
  uint64_t offset = 0;
  for (; offset + block_size < size; offset += block_size, ++link) {
    PReadOrThrow(file_, link->Get(), block_size, offset);
    link->SetValidSize(block_size);
  }
  if (size - offset) {
    PReadOrThrow(file_, link->Get(), size - offset, offset);
    link->SetValidSize(size - offset);
    ++link;
  }
  link.Poison();
}

void Write::Run(const ChainPosition &position) {
  for (Link link(position); link; ++link) {
    util::WriteOrThrow(file_, link->Get(), link->ValidSize());
  }
}

} // namespace stream
} // namespace util