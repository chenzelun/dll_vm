//
// Created by 陈泽伦 on 11/16/20.
//

#ifndef VM_ANDROIDSOURCE_H
#define VM_ANDROIDSOURCE_H


#include <cstdint>
#include <atomic>
#include <string>

/*
 * These match the definitions in the VM specification.
 */
typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;
typedef int8_t s1;
typedef int16_t s2;
typedef int32_t s4;
typedef int64_t s8;

/*
 * Header added by DEX optimization pass.  Values are always written in
 * local byte and structure padding.  The first field (magic + version)
 * is guaranteed to be present and directly readable for all expected
 * compiler configurations; the rest is version-dependent.
 *
 * Try to keep this simple and fixed-size.
 */
struct DexOptHeader {
    u1 magic[8];           /* includes version number */

    u4 dexOffset;          /* file offset of DEX header */
    u4 dexLength;
    u4 depsOffset;         /* offset of optimized DEX dependency table */
    u4 depsLength;
    u4 optOffset;          /* file offset of optimized data tables */
    u4 optLength;

    u4 flags;              /* some info flags */
    u4 checksum;           /* adler32 checksum covering deps/opt */

    /* pad for 64-bit alignment if necessary */
};

/*
 * 160-bit SHA-1 digest.
 */
const uint32_t kSHA1DigestLen = 20;

/*
 * Direct-mapped "header_item" struct.
 */
struct DexHeader {
    u1 magic[8];           /* includes version number */
    u4 checksum;           /* adler32 checksum */
    u1 signature[kSHA1DigestLen]; /* SHA-1 hash */
    u4 fileSize;           /* length of entire file */
    u4 headerSize;         /* offset to start of next section */
    u4 endianTag;
    u4 linkSize;
    u4 linkOff;
    u4 mapOff;
    u4 stringIdsSize;
    u4 stringIdsOff;
    u4 typeIdsSize;
    u4 typeIdsOff;
    u4 protoIdsSize;
    u4 protoIdsOff;
    u4 fieldIdsSize;
    u4 fieldIdsOff;
    u4 methodIdsSize;
    u4 methodIdsOff;
    u4 classDefsSize;
    u4 classDefsOff;
    u4 dataSize;
    u4 dataOff;
};

/*
 * Direct-mapped "string_id_item".
 */
struct DexStringId {
    u4 stringDataOff;      /* file offset to string_data_item */
};

/*
 * Direct-mapped "type_id_item".
 */
struct DexTypeId {
    u4 descriptorIdx;      /* index into stringIds list for type descriptor */
};

/*
 * Direct-mapped "field_id_item".
 */
struct DexFieldId {
    u2 classIdx;           /* index into typeIds list for defining class */
    u2 typeIdx;            /* index into typeIds for field type */
    u4 nameIdx;            /* index into stringIds for field name */
};

/*
 * Direct-mapped "method_id_item".
 */
struct DexMethodId {
    u2 classIdx;           /* index into typeIds list for defining class */
    u2 protoIdx;           /* index into protoIds for method prototype */
    u4 nameIdx;            /* index into stringIds for method name */
};

/*
 * Direct-mapped "proto_id_item".
 */
struct DexProtoId {
    u4 shortyIdx;          /* index into stringIds for shorty descriptor */
    u4 returnTypeIdx;      /* index into typeIds list for return type */
    u4 parametersOff;      /* file offset to type_list for parameter types */
};

/*
 * Direct-mapped "class_def_item".
 */
struct DexClassDef {
    u4 classIdx;           /* index into typeIds for this class */
    u4 accessFlags;
    u4 superclassIdx;      /* index into typeIds for superclass */
    u4 interfacesOff;      /* file offset to DexTypeList */
    u4 sourceFileIdx;      /* index into stringIds for source file name */
    u4 annotationsOff;     /* file offset to annotations_directory_item */
    u4 classDataOff;       /* file offset to class_data_item */
    u4 staticValuesOff;    /* file offset to DexEncodedArray */
};

/*
 * Direct-mapped "type_item".
 */
struct DexTypeItem {
    u2 typeIdx;            /* index into typeIds */
};

/*
 * Direct-mapped "type_list".
 */
struct DexTypeList {
    u4 size;               /* #of entries in list */
    DexTypeItem list[1];    /* entries */
};

struct CodeItemData {
    u2 registersSize;
    u2 insSize;
    u2 outSize;
    u2 triesSize;
    u4 debugInfoOff;
    u4 insnsSize;
    u2 insns[1];
    u1 *triesAndHandlersBuf;
};

class ArtMethod_26_28 {
public:
// Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
    // The class we are a part of.
    uint32_t declaring_class;

    // Access flags; low 16 bits are defined by spec.
    // Getting and setting this flag needs to be atomic when concurrency is
    // possible, e.g. after this method's class is linked. Such as when setting
    // verifier flags and single-implementation flag.
    std::atomic<uint32_t> access_flags;

    /* Dex file fields. The defining dex file is available via declaring_class_->dex_cache_ */

    // Offset to the CodeItem.
    uint32_t dex_code_item_offset;

    // Index into method_ids of the dex file associated with this method.
    uint32_t dex_method_index;

    /* End of dex file fields. */

    // Entry within a dispatch table for this method. For static/direct methods the index is into
    // the declaringClass.directMethods, for virtual methods the vtable and for interface methods the
    // ifTable.
    uint16_t method_index;

    // The hotness we measure for this method. Managed by the interpreter. Not atomic, as we allow
    // missing increments: if the method is hot, we will see it eventually.
    uint16_t hotness_count;

    // Fake padding field gets inserted here.
};

#define PACKED(x) __attribute__ ((__aligned__(x), __packed__))
#define MANAGED PACKED(4)

template<typename T>
class PACKED(sizeof(T)) Atomic : public std::atomic<T> {

};

class ArtDexFile {
public:
    const void *virtualMethod;

    // The base address of the memory mapping.
    const uint8_t *const begin;

    // The size of the underlying memory allocation in bytes.
    const size_t size;

    // Typically the dex file name when available, alternatively some identifying string.
    //
    // The ClassLinker will use this to match DexFiles the boot class
    // path to DexCache::GetLocation when loading from an image.
    const std::string location;

    const uint32_t location_checksum;

    // Manages the underlying memory allocation.
    void *mem_map;

    // Points to the header section.
    const void *const header;

    // Points to the base of the string identifier list.
    const void *const string_ids;

    // Points to the base of the type identifier list.
    const void *const type_ids;

    // Points to the base of the field identifier list.
    const void *const field_ids;

    // Points to the base of the method identifier list.
    const void *const method_ids;

    // Points to the base of the prototype identifier list.
    const void *const proto_ids;

    // Points to the base of the class definition list.
    const void *const class_defs;

    // Number of misses finding a class def from a descriptor.
    mutable Atomic<uint32_t> find_class_def_misses;

    mutable Atomic<void *> class_def_index;

    // If this dex file was loaded from an oat file, oat_dex_file_ contains a
    // pointer to the OatDexFile it was loaded from. Otherwise oat_dex_file_ is
    // null.
    const void *oat_dex_file;
};

// C++ mirror of java.lang.Object
class MANAGED ArtObject {
public:
    // The Class representing the type of the object.
    uint32_t klass;
    // Monitor and hash code information.
    uint32_t monitor;

};


// C++ mirror of java.lang.Class
class MANAGED ArtClass : public ArtObject {
public:
    // Defining class loader, or null for the "bootstrap" system loader.
    uint32_t class_loader;

    // For array classes, the component class object for instanceof/checkcast
    // (for String[][][], this will be String[][]). null for non-array classes.
    uint32_t component_type;

    // DexCache of resolved constant pool entries (will be null for classes generated by the
    // runtime such as arrays and primitive classes).
    uint32_t dex_cache;

    // Short cuts to dex_cache_ member for fast compiled code access.
    uint32_t dex_cache_strings;

    // The interface table (iftable_) contains pairs of a interface class and an array of the
    // interface methods. There is one pair per interface supported by this class.  That means one
    // pair for each interface we support directly, indirectly via superclass, or indirectly via a
    // superinterface.  This will be null if neither we nor our superclass implement any interfaces.
    //
    // Why we need this: given "class Foo implements Face", declare "Face faceObj = new Foo()".
    // Invoke faceObj.blah(), where "blah" is part of the Face interface.  We can't easily use a
    // single vtable.
    //
    // For every interface a concrete class implements, we create an array of the concrete vtable_
    // methods for the methods in the interface.
    uint32_t iftable;

    // Descriptor for the class such as "java.lang.Class" or "[C". Lazily initialized by ComputeName
    uint32_t name;

    // The superclass, or null if this is java.lang.Object, an interface or primitive type.
    uint32_t super_class;

    // If class verify fails, we must return same error on subsequent tries.
    uint32_t verify_error_class;

    // Virtual method table (vtable), for use by "invoke-virtual".  The vtable from the superclass is
    // copied in, and virtual methods from our class either replace those from the super or are
    // appended. For abstract classes, methods may be created in the vtable that aren't in
    // virtual_ methods_ for miranda methods.
    uint32_t vtable;

    // Access flags; low 16 bits are defined by VM spec.
    // Note: Shuffled back.
    uint32_t access_flags;

    // static, private, and <init> methods. Pointer to an ArtMethod array.
    uint64_t direct_methods;

    // instance fields
    //
    // These describe the layout of the contents of an Object.
    // Note that only the fields directly declared by this class are
    // listed in ifields; fields declared by a superclass are listed in
    // the superclass's Class.ifields.
    //
    // ArtField arrays are allocated as an array of fields, and not an array of fields pointers.
    uint64_t ifields;

    // Static fields
    uint64_t sfields;

    // Virtual methods defined in this class; invoked through vtable. Pointer to an ArtMethod array.
    uint64_t virtual_methods;

    // Total size of the Class instance; used when allocating storage on gc heap.
    // See also object_size_.
    uint32_t class_size;

    // Tid used to check for recursive <clinit> invocation.
    pid_t clinit_thread_id;

    // ClassDef index in dex file, -1 if no class definition such as an array.
    int32_t dex_class_def_idx;

    // Type index in dex file.
    int32_t dex_type_idx;

    // Number of direct fields.
    uint32_t num_direct_methods;

    // Number of instance fields.
    uint32_t num_instance_fields;

    // Number of instance fields that are object refs.
    uint32_t num_reference_instance_fields;

    // Number of static fields that are object refs,
    uint32_t num_reference_static_fields;

    // Number of static fields.
    uint32_t num_static_fields;

    // Number of virtual methods.
    uint32_t num_virtual_methods;

    // Total object size; used when allocating storage on gc heap.
    // (For interfaces and abstract classes this will be zero.)
    // See also class_size_.
    uint32_t object_size;

    // The lower 16 bits contains a Primitive::Type value. The upper 16
    // bits contains the size shift of the primitive type.
    uint32_t primitive_type;

    // Bitmap of offsets of ifields.
    uint32_t reference_instance_offsets;
};


class MANAGED ArtDexCache_26_28 : public ArtObject {
public:
    uint32_t location;
    // Number of elements in the call_sites_ array. Note that this appears here
    // because of our packing logic for 32 bit fields.
    uint32_t num_resolved_call_sites;

    uint64_t dex_file;
    uint64_t resolved_call_sites;

    uint64_t resolved_fields;
    uint64_t resolved_method_types;
    uint64_t resolved_methods;
    uint64_t resolved_types;
    uint64_t strings;

    uint32_t num_resolved_fields;
    uint32_t num_resolved_method_types;
    uint32_t num_resolved_methods;
    uint32_t num_resolved_types;
    uint32_t num_strings;
};


/*
 * access flags and masks; the "standard" ones are all <= 0x4000
 *
 * Note: There are related declarations in vm/oo/Object.h in the ClassFlags
 * enum.
 */
enum {
    ACC_PUBLIC = 0x00000001,       // class, field, method, ic
    ACC_PRIVATE = 0x00000002,       // field, method, ic
    ACC_PROTECTED = 0x00000004,       // field, method, ic
    ACC_STATIC = 0x00000008,       // field, method, ic
    ACC_FINAL = 0x00000010,       // class, field, method, ic
    ACC_SYNCHRONIZED = 0x00000020,       // method (only allowed on natives)
    ACC_SUPER = 0x00000020,       // class (not used in Dalvik)
    ACC_VOLATILE = 0x00000040,       // field
    ACC_BRIDGE = 0x00000040,       // method (1.5)
    ACC_TRANSIENT = 0x00000080,       // field
    ACC_VARARGS = 0x00000080,       // method (1.5)
    ACC_NATIVE = 0x00000100,       // method
    ACC_INTERFACE = 0x00000200,       // class, ic
    ACC_ABSTRACT = 0x00000400,       // class, method, ic
    ACC_STRICT = 0x00000800,       // method
    ACC_SYNTHETIC = 0x00001000,       // field, method, ic
    ACC_ANNOTATION = 0x00002000,       // class, ic (1.5)
    ACC_ENUM = 0x00004000,       // class, field, ic (1.5)
    ACC_CONSTRUCTOR = 0x00010000,       // method (Dalvik only)
    ACC_DECLARED_SYNCHRONIZED =
    0x00020000,       // method (Dalvik only)
    ACC_CLASS_MASK =
    (ACC_PUBLIC | ACC_FINAL | ACC_INTERFACE | ACC_ABSTRACT
     | ACC_SYNTHETIC | ACC_ANNOTATION | ACC_ENUM),
    ACC_INNER_CLASS_MASK =
    (ACC_CLASS_MASK | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC),
    ACC_FIELD_MASK =
    (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
     | ACC_VOLATILE | ACC_TRANSIENT | ACC_SYNTHETIC | ACC_ENUM),
    ACC_METHOD_MASK =
    (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
     | ACC_SYNCHRONIZED | ACC_BRIDGE | ACC_VARARGS | ACC_NATIVE
     | ACC_ABSTRACT | ACC_STRICT | ACC_SYNTHETIC | ACC_CONSTRUCTOR
     | ACC_DECLARED_SYNCHRONIZED),
};

#endif //VM_ANDROIDSOURCE_H
