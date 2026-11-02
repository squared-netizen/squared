export module sq.graphics:src.texture_recovery_callback;

export import :src.texture_recovery_target;

export namespace sq::graphics
{

using TextureRecoveryCallback = bool ( * )( void *user_data,
                                            TextureRecoveryTarget &target ) noexcept;

} // namespace sq::graphics
