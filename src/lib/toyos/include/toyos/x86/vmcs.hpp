/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/x86/segmentation.hpp>
#include <toyos/x86/vmxasm.hpp>
#include <toyos/x86/x86defs.hpp>

namespace vmcs
{

    /**
 * \brief Virtual machine control structure.
 *
 * This data structure is used by the processor while it is in VMX operation.
 * When it is referenced by a VMCS pointer, make sure to align it to a 4 KByte boundary (Intel SDM, Vol. 3, 24.1).
 *
 * The size of a VMCS is defined by the hardware and it can be up to 4 KByte. This implementation reserves exactly
 * 4 KByte.
 */
    class alignas(PAGE_SIZE) vmcs_t
    {
     private:
        union
        {
            uint8_t raw[PAGE_SIZE];
            uint32_t rev_id;
        } raw;

     public:
        /**
     * \brief  Create a new VMCS.
     *
     * Information should be read from or written to a VMCS by using vmxread and vmxwrite.
     * Copying and moving of the data structure are therefore prohibited.
     */
        vmcs_t() = default;
        vmcs_t(const vmcs_t& other) = delete;
        vmcs_t(vmcs_t&& other) = delete;

        /**
     * \brief Clear content of VMCS with zeroes.
     */
        void clear()
        {
            raw = {};
        }

        /**
     * \brief Write a revision identifier to the start of the VMCS region.
     *
     * \param rev_id_ revision identifier
     */
        void set_rev_id(uint32_t rev_id_)
        {
            raw.rev_id = rev_id_;
        }

        /**
     * \brief Read a VMCS field.
     *
     * \param enc field encoding
     *
     * \return value
     */
        uint64_t read(x86::vmcs_encoding enc)
        {
            return vmread(enc);
        }

        /**
     * \brief Write a VMCS field.
     *
     * \param enc field encoding
     * \param val field value
     */
        void write(x86::vmcs_encoding enc, uint64_t val)
        {
            vmwrite(enc, val);
        }

        /**
     * \brief Set IP to next instruction.
     *
     * Increases guest RIP in a VMCS by the instruction length encoded in this VMCS if the length is not 0.
     * Note that only selected VM exits due to instruction execution set this field (see Intel SDM, Vol. 3, 27.2.4).
     *
     * \param vmcs_VMCS
     */
        void adjust_rip()
        {
            auto rip{ read(x86::vmcs_encoding::GUEST_RIP) };
            auto len{ read(x86::vmcs_encoding::VM_EXI_INS_LEN) };
            assert(len > 0);
            write(x86::vmcs_encoding::GUEST_RIP, rip + len);
        }
    };

    /**
 * \brief Segment access rights in a VMCS.
 *
 * Their layout differs from access rights in a GDT entry as follows:
 *  - an unusable bit, indicating whether a segment is usable, exists in VMCS, but not in GDT entries
 *  - it starts at bit 0 (0-3 are segment type). In GDT entry, the entry starts at bit 8.
 */
    class access_rights
    {
        using gdt_entry = x86::gdt_entry;

     public:
        /**
     * \brief Initialize access rights from a GDT entry.
     *
     * \para gdte GDT entry
     */
        explicit access_rights(uint64_t gdte)
        {
            raw = ((gdte >> gdt_entry::AR_LO_SHIFT)
                   & math::mask(gdt_entry::AR_LO_WIDTH + gdt_entry::LIMIT_HI_WIDTH + gdt_entry::AR_HI_WIDTH));

            raw &= ~(math::mask(RESERVED0_WIDTH) << RESERVED0_SHIFT);
            raw &= ~(math::mask(RESERVED1_WIDTH) << RESERVED1_SHIFT);
        }

        uint8_t get_type() const
        {
            return get_bits(TYPE_SHIFT, TYPE_WIDTH);
        }
        void set_type(uint8_t v)
        {
            set_bits(v, TYPE_SHIFT, TYPE_WIDTH);
        }

        bool get_system() const
        {
            return get_bits(SYSTEM_SHIFT);
        }
        void set_system(bool v)
        {
            set_bits(v, SYSTEM_SHIFT);
        }

        uint8_t get_dpl() const
        {
            return get_bits(DPL_SHIFT);
        }
        void set_dpl(bool v)
        {
            set_bits(v, DPL_SHIFT);
        }

        bool get_present() const
        {
            return get_bits(PRESENT_SHIFT);
        }
        void set_present(bool v)
        {
            set_bits(v, PRESENT_SHIFT);
        }

        bool get_avl() const
        {
            return get_bits(AVL_SHIFT);
        }
        void set_avl(bool v)
        {
            set_bits(v, AVL_SHIFT);
        }

        bool get_cs_long() const
        {
            return get_bits(CS_LONG_SHIFT);
        }
        void set_cs_long(bool v)
        {
            set_bits(v, CS_LONG_SHIFT);
        }

        bool get_size() const
        {
            return get_bits(SIZE_SHIFT);
        }
        void set_size(bool v)
        {
            set_bits(v, SIZE_SHIFT);
        }

        bool get_granularity() const
        {
            return get_bits(GRANULARITY_SHIFT);
        }
        void set_granularity(bool v)
        {
            set_bits(v, GRANULARITY_SHIFT);
        }

        bool get_unusable() const
        {
            return get_bits(UNUSABLE_SHIFT);
        }
        void set_unusable(bool v)
        {
            set_bits(v, UNUSABLE_SHIFT);
        }

        uint32_t get_value() const
        {
            return raw;
        }

     private:
        void set_bits(uint32_t bits, uint32_t shift, uint32_t width = 1)
        {
            raw = (raw & ~(math::mask(width) << shift)) | (bits << shift);
        }
        uint32_t get_bits(uint32_t shift, uint32_t width = 1) const
        {
            return (raw >> shift) & math::mask(width);
        }

        enum
        {
            TYPE_SHIFT = 0,
            TYPE_WIDTH = 4,
            SYSTEM_SHIFT = 4,
            DPL_SHIFT = 5,
            DPL_WIDTH = 2,
            PRESENT_SHIFT = 7,
            RESERVED0_SHIFT = 8,
            RESERVED0_WIDTH = 4,
            AVL_SHIFT = 12,
            CS_LONG_SHIFT = 13,
            SIZE_SHIFT = 14,
            GRANULARITY_SHIFT = 15,
            UNUSABLE_SHIFT = 16,
            RESERVED1_SHIFT = 17,
            RESERVED1_WIDTH = 14,
        };

        uint32_t raw;
    };

    /**
 * \brief Helper for turning segmentation information from a GDT into a VMCS segment description format.
 *
 * To setup segmentation information in a VMCS requires multiple writes and the format
 * varies significantly from the one used in the GDT. This helper allows to read information from a GDT, turns it
 * into a VMCS format and caches the information. Changing information in this data structure does neither
 * update a GDT entry nor a VMCS entry.
 *
 * Keep in mind that the segmentation description in a VMCS differs for guest and host state and is as follows
 * (Intel SDM, Vol. 3, Appendix B):
 *
 * Guest state:
 *   ES   | selector (16 bit) | base (64 bit) | limit (32 bit) | access rights (32 bit)
 *   CS   | selector (16 bit) | base (64 bit) | limit (32 bit) | access rights (32 bit)
 *   SS   | selector (16 bit) | base (64 bit) | limit (32 bit) | access rights (32 bit)
 *   DS   | selector (16 bit) | base (64 bit) | limit (32 bit) | access rights (32 bit)
 *   FS   | selector (16 bit) | base (64 bit) | limit (32 bit) | access rights (32 bit)
 *   GS   | selector (16 bit) | base (64 bit) | limit (32 bit) | access rights (32 bit)
 *   LDTR | selector (16 bit) | base (64 bit) | limit (32 bit) | access rights (32 bit)
 *   TR   | selector (16 bit) | base (64 bit) | limit (32 bit) | access rights (32 bit)
 *   GDTR |                   | base (64 bit) | limit (32 bit) |
 *   IDTR |                   | base (64 bit) | limit (32 bit) |
 *
 * Host state:
 *   ES   | selector (16 bit) |
 *   CS   | selector (16 bit) |
 *   SS   | selector (16 bit) |
 *   DS   | selector (16 bit) |
 *   FS   | selector (16 bit) | base (64 bit)
 *   GS   | selector (16 bit) | base (64 bit)
 *   TR   | selector (16 bit) | base (64 bit)
 *   GDTR |                   | base (64 bit)
 *   IDTR |                   | base (64 bit)
 *
 */
    class cached_segment_descriptor
    {
        using gdt_entry = x86::gdt_entry;

     public:
        /**
     * \brief Create a cached entry based for the selected segment using a global descriptor table register pointer.
     *
     * The access rights in a VMCS have an additional bit which is absent in the GDT: the unusable bit.
     * It is automatically set to true when selector value of 0 is given.
     *
     * Be careful when using this helper for TR. It is usable after processor reset
     * despite having a null selector (Intel SDM, Vol. 3, 24.4.1, Footnote 2). There is no context here to identify
     * that TR is handled here and it will be set to unusable.
     *
     * \param gdtr global descriptor table register pointer
     * \param selector_ segment selector
     */
        cached_segment_descriptor(x86::descriptor_ptr gdtr, uint16_t selector_)
            : selector(selector_)
        {
            assert(selector < gdtr.limit);

            gdt_entry* gdte{ get_gdt_entry(gdtr, x86::segment_selector{ selector_ }) };

            base = gdte->base32();
            limit = gdte->limit();

            ar = access_rights(gdte->raw);

            // A segment is unusable if it has been loaded with a null selector.
            // A segment might also be unusable if its selector is not null after a task switch failed
            // (Intel SDM, Vol. 3, 24.4.1, Footnote 2). Task switches are not anticipated here.
            if (selector_ == 0) {
                ar.set_unusable(true);
            }
        }

        uint16_t selector;
        uint64_t base;
        uint32_t limit;
        access_rights ar{ 0 };
    };

}  // namespace vmcs
