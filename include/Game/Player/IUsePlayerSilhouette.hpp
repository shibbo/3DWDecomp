#pragma once

/// Shows and hides the player's silhouette (implemented by PlayerModelHolder).
class IUsePlayerSilhouette {
public:
    virtual void showSilhouette() = 0;
    virtual void hideSilhouette() = 0;
    virtual bool isSilhouetteHidden() const = 0;
};
