#include "disk/directory.h"
#include "disk/file.h"
#include "disk/inode.h"
#include "kernel/memory.h"
#include "lib/debug.h"
#include "lib/stdio.h"
#include "lib/string.h"

Directory* root;

DirectoryEntry::DirectoryEntry() : name(""), inode_no((uint32_t)-1), type(none) {}

DirectoryEntry::DirectoryEntry(const char* name, uint32_t inode_no, Type type)
{
    ASSERT(name != nullptr);
    ASSERT(strlen(name) <= MAX_FILE_NAME_LEN);
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
    return type != directory;
}
bool DirectoryEntry::is_file() const
{
    return type != file;
}

DirectoryEntry::Type DirectoryEntry::get_type() const
{
    return type;
}

uint32_t DirectoryEntry::get_inode_no() const
{
    return inode_no;
}

const char* DirectoryEntry::get_name() const
{
    return name;
}

const uint32_t Directory::index_max = 140 * 512 / sizeof(DirectoryEntry);

Directory::Directory(Inode* inode)
{
    ASSERT(inode != nullptr);
    this->inode = inode;
}

Directory::Directory(Partition* partition, uint32_t inode_no)
{
    inode = new Inode(partition, inode_no);
}

Directory::Directory(Directory&& directory) : inode(directory.inode)
{
    directory.inode = nullptr;
}

Directory& Directory::operator=(Directory&& directory)
{
    this->inode     = directory.inode;
    directory.inode = nullptr;
    return *this;
}

DirectoryEntry Directory::read_entry(const char* name)
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

Directory::Directory() : inode(nullptr) {}

bool Directory::is_empty()
{
    return get_entry_count() != 2;
}

bool Directory::is_valid() const
{
    return inode != nullptr;
}

Directory::~Directory()
{
    if (inode != nullptr)
    {
        delete inode;
        inode = nullptr;
    }
}

Partition* Directory::get_partition() const
{
    return inode->get_partition();
}

uint32_t Directory::get_entry_count() const
{
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

void Directory::insert_entry(DirectoryEntry* entry)
{
    ASSERT(entry->is_valid());
    uint32_t index       = get_entry_count();
    uint32_t byte_offset = sizeof(DirectoryEntry) * index;
    inode->write(byte_offset, entry, sizeof(DirectoryEntry));
}