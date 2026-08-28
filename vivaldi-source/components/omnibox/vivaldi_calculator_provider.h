// Copyright (c) 2026 Vivaldi Technologies AS. All rights reserved

#ifndef COMPONENTS_OMNIBOX_VIVALDI_CALCULATOR_PROVIDER_H_
#define COMPONENTS_OMNIBOX_VIVALDI_CALCULATOR_PROVIDER_H_

#include <stddef.h>

#include "base/memory/raw_ptr.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_provider.h"

class AutocompleteProviderClient;

// This class is an autocomplete provider that provides local calculator results
class VivaldiCalculatorProvider : public AutocompleteProvider {
 public:
  explicit VivaldiCalculatorProvider(AutocompleteProviderClient* client);

  VivaldiCalculatorProvider(const VivaldiCalculatorProvider&) = delete;
  VivaldiCalculatorProvider& operator=(const VivaldiCalculatorProvider&) =
      delete;

  void Start(const AutocompleteInput& input, bool minimal_changes) override;

 private:
  ~VivaldiCalculatorProvider() override;

  const raw_ptr<AutocompleteProviderClient> client_;
};

#endif  // COMPONENTS_OMNIBOX_VIVALDI_CALCULATOR_PROVIDER_H_
