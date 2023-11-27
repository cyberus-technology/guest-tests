# Embeds a guest test as Multiboot2 ELF in GRUB as EFI-loader.
# The result is a direct symlink to the boot item.

{ grub2_efi
, runCommand
, writeText

, name
, kernel
, kernelCmdline ? "--serial"
}:

let
  grubCfg = writeText "${name}-grub.cfg" ''
    set timeout=0
    menuentry 'Test' {
      # We need multiboot2 to get the ACPI RSDP.
      multiboot2 /boot/kernel ${kernelCmdline}
    }
  '';
in
runCommand name
{
  nativeBuildInputs = [ grub2_efi ];
  passthru = {
    inherit grubCfg;
  };
} ''
  # make a memdisk-based GRUB image
  grub-mkstandalone \
    --format x86_64-efi \
    --output $out \
    --directory ${grub2_efi}/lib/grub/x86_64-efi \
    "/boot/grub/grub.cfg=${grubCfg}" \
    "/boot/kernel=${kernel}"
    # ^ This is poorly documented, but the tool allows to specify key-value
    # pairs where the value on the right, a file, will be embedded into the
    # "(memdisk)" volume inside the grub image. -> "Graft point syntax"
''
