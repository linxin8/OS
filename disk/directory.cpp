#include "disk/directory.h"
#include "disk/file.h"
#include "disk/inode.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/stdio.h"
#include "lib/string.h"

DirectoryEntry::DirectoryEntry() : name(""), inode_no(-1), type(none) {}

DirectoryEntry::DirectoryEntry(const char* name, int32_t inode_no, Type type)
{
    ASSERT(name != nullptr);
    ASSERT(strlen(name) <= MAX_FILE_NAME_LEN);
    memset((void*)this->name, 0, sizeof(this->name));
    strcpy(this->name, name);
    this->inode_no = inode_no;
    this->type     = type;
}

bool DirectoryEntry::is_valid() const
{
    return type != none;
}
bool DirectoryEntry::is_directory() const
{
    return type == directory;
}
bool DirectoryEntry::is_file() const
{
    return type == file;
}

DirectoryEntry::Type DirectoryEntry::get_type() const
{
    return type;
}

int32_t DirectoryEntry::get_inode_no() const
{
    return inode_no;
}

const char* DirectoryEntry::get_name() const
{
    return name;
}

DirectoryEntry Directory::find_entry(const char* name)
{
    uint32_t count = get_entry_count();
    for (uint32_t i = 0; i < count; i++)
    {
        auto entry = read_entry(i);
        if (entry.is_directory())
        {  //是目录
            if (strcmp(entry.get_name(), name) == 0)
            {
                return entry;
            }
        }
    }
    return DirectoryEntry();
}

const uint32_t Directory::index_max = 140 * 512 / sizeof(DirectoryEntry);

Directory::Directory(Inode* inode) : inode(Inode::copy_instance(inode)) {}

Directory Directory::open_root_directory(Partition* partition)
{
    return Directory(partition, 0);
}

Directory::Directory(Partition* partition, int32_t inode_no) : inode(Inode::get_instance(partition, inode_no)) {}

Directory::Directory() : inode(nullptr) {}

Directory::Directory(const Directory& directory) : inode(Inode::copy_instance(directory.inode)) {}

Directory::~Directory()
{
    if (inode != nullptr)
    {
        Inode::remove_instance(inode);
        inode = nullptr;
    }
}

void Directory::insert_directory(const char* name)
{
    if (find_entry(name).is_valid())
    {
        printk("%s is already exist\n", name);
    }
    else
    {
        auto no = inode->get_partition()->alloc_inode();
        insert_entry(DirectoryEntry{name, no, DirectoryEntry::directory});
        auto directory = open_directory(name);
        ASSERT(directory.is_valid());
        directory.insert_entry(DirectoryEntry{".", no, DirectoryEntry::directory});
        directory.insert_entry(DirectoryEntry{"..", inode->get_no(), DirectoryEntry::directory});
    }
}

void Directory::insert_file(const char* name)
{
    if (find_entry(name).is_valid())
    {
        printk("%s is already exist\n", name);
    }
    else
    {
        insert_entry(DirectoryEntry{name, inode->get_partition()->alloc_inode(), DirectoryEntry::file});
    }
}

bool Directory::is_empty()
{
    return get_entry_count() != 2;
}

bool Directory::is_valid() const
{
    return inode != nullptr;
}

Partition* Directory::get_partition() const
{
    return inode->get_partition();
}

uint32_t Directory::get_entry_count() const
{
    ASSERT(inode != nullptr);
    ASSERT(inode->get_size() % sizeof(DirectoryEntry) == 0);
    return inode->get_size() / sizeof(DirectoryEntry);
}

DirectoryEntry Directory::read_entry(uint32_t index)
{
    ASSERT(index < get_entry_count());
    DirectoryEntry entry;
    uint32_t       byte_offset = sizeof(DirectoryEntry) * index;
    inode->read(byte_offset, &entry, sizeof(DirectoryEntry));
    ASSERT(entry.is_valid());  //确保目录项存在
    return entry;
}

void Directory::write_entry(uint32_t index, const DirectoryEntry& entry)
{
    ASSERT(index < get_entry_count());
    ASSERT(entry.is_valid());  //确保目录项存在
    uint32_t byte_offset = sizeof(DirectoryEntry) * index;
    inode->write(byte_offset, &entry, sizeof(DirectoryEntry));
}

void Directory::insert_entry(const DirectoryEntry& entry)
{
    ASSERT(entry.is_valid());
    uint32_t index       = get_entry_count();
    uint32_t byte_offset = sizeof(DirectoryEntry) * index;
    inode->write(byte_offset, &entry, sizeof(DirectoryEntry));
}

Directory Directory::open_directory(const char* name)
{
    auto entry = find_entry(name);
    if (entry.is_valid() && entry.is_directory())
    {
        return Directory(inode->get_partition(), entry.get_inode_no());
    }
    return Directory();
}
