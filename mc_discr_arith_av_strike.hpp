/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2008 Master IMAFA - Polytech'Nice Sophia - Université de Nice Sophia Antipolis

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file mc_discr_arith_av_strike.hpp
    \brief Monte Carlo engine for discrete arithmetic average-strike Asian
*/

#ifndef mc_discrete_arithmetic_average_strike_asian_engine_hpp
#define mc_discrete_arithmetic_average_strike_asian_engine_hpp

#include <ql/exercise.hpp>
#include <ql/pricingengines/asian/mcdiscreteasianenginebase.hpp>
#include <ql/pricingengines/asian/mc_discr_arith_av_strike.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <utility>

namespace QuantLib {

    //!  Monte Carlo pricing engine for discrete arithmetic average-strike Asian
    /*!  \ingroup asianengines */
    template <class RNG = PseudoRandom, class S = Statistics>
    class MCDiscreteArithmeticASEngine_2
        : public MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S> {
      public:
        typedef
        typename MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S>::path_generator_type
            path_generator_type;
        typedef typename MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S>::path_pricer_type
            path_pricer_type;
        typedef typename MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S>::stats_type
            stats_type;
        // constructor
        MCDiscreteArithmeticASEngine_2(
             const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
             bool brownianBridge,
             bool antitheticVariate,
             Size requiredSamples,
             Real requiredTolerance,
             Size maxSamples,
             BigNatural seed,
             bool constantParameters);

    private:
        bool constantParameters;

        // Override the path generator
        ext::shared_ptr<path_generator_type> pathGenerator() const override {

            Size dimensions = MCDiscreteAveragingAsianEngineBase<SingleVariate, RNG, S>::process_->factors();
            TimeGrid grid = this->timeGrid();
            typename RNG::rsg_type generator =
                RNG::make_sequence_generator(dimensions * (grid.size() - 1), MCDiscreteAveragingAsianEngineBase<SingleVariate, RNG, S>::seed_);

            //bool constantParameters=true;
            if (this->constantParameters)
            {
                ext::shared_ptr<GeneralizedBlackScholesProcess> BS_process =
                    ext::dynamic_pointer_cast<GeneralizedBlackScholesProcess>(this->process_);

                // Get the parameters from the ConstantBlackScholesProcess class
                Time time_of_extraction = grid.back();
                double strike = ext::dynamic_pointer_cast<StrikedTypePayoff>(MCDiscreteAveragingAsianEngineBase<SingleVariate, RNG, S>::arguments_.payoff)->strike();
                double riskFreeRate_ = BS_process->riskFreeRate()->zeroRate(time_of_extraction, Continuous);
                double dividend_ = BS_process->dividendYield()->zeroRate(time_of_extraction, Continuous);
                double volatility_ = BS_process->blackVolatility()->blackVol(time_of_extraction, strike);
                double underlyingValue_ = BS_process->x0();

                // We instanciate a constantBSProcess named cst_BS_process with the parameters
                ext::shared_ptr<ConstantBlackScholesProcess> cst_BS_process(new ConstantBlackScholesProcess(underlyingValue_, riskFreeRate_, volatility_, dividend_));

                // We return a new path generator with constantBSProcess
                return ext::shared_ptr<path_generator_type>(
                    new path_generator_type(cst_BS_process, grid,
                        generator, MCDiscreteAveragingAsianEngineBase<SingleVariate, RNG, S>::brownianBridge_));
            }

            else{ // We return the classical path generator 

                return ext::shared_ptr<path_generator_type>(
                    new path_generator_type(MCDiscreteAveragingAsianEngineBase<SingleVariate, RNG, S>::process_, grid,
                        generator, MCDiscreteAveragingAsianEngineBase<SingleVariate, RNG, S>::brownianBridge_));
            }
        }


      protected:
        ext::shared_ptr<path_pricer_type> pathPricer() const override;
    };


    // inline definitions

    template <class RNG, class S>
    inline
    MCDiscreteArithmeticASEngine_2<RNG,S>::MCDiscreteArithmeticASEngine_2(
             const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
             bool brownianBridge,
             bool antitheticVariate,
             Size requiredSamples,
             Real requiredTolerance,
             Size maxSamples,
             BigNatural seed,
             bool constantParameters)
    : MCDiscreteAveragingAsianEngineBase<SingleVariate,RNG,S>(process,
                                                              brownianBridge,
                                                              antitheticVariate,
                                                              false,
                                                              requiredSamples,
                                                              requiredTolerance,
                                                              maxSamples,
                                                              seed) {
                this->constantParameters = constantParameters;

    }

    template <class RNG, class S>
    inline
    ext::shared_ptr<
               typename MCDiscreteArithmeticASEngine_2<RNG,S>::path_pricer_type>
    MCDiscreteArithmeticASEngine_2<RNG,S>::pathPricer() const {

        ext::shared_ptr<PlainVanillaPayoff> payoff =
            ext::dynamic_pointer_cast<PlainVanillaPayoff>(
                this->arguments_.payoff);
        QL_REQUIRE(payoff, "non-plain payoff given");

        ext::shared_ptr<EuropeanExercise> exercise =
            ext::dynamic_pointer_cast<EuropeanExercise>(
                this->arguments_.exercise);
        QL_REQUIRE(exercise, "wrong exercise given");

        ext::shared_ptr<GeneralizedBlackScholesProcess> process =
            ext::dynamic_pointer_cast<GeneralizedBlackScholesProcess>(
                this->process_);
        QL_REQUIRE(process, "Black-Scholes process required");

        return ext::shared_ptr<typename
            MCDiscreteArithmeticASEngine_2<RNG,S>::path_pricer_type>(
                new ArithmeticASOPathPricer(
                    payoff->optionType(),
                    process->riskFreeRate()->discount(exercise->lastDate()),
                    this->arguments_.runningAccumulator,
                    this->arguments_.pastFixings));
    }



    template <class RNG = PseudoRandom, class S = Statistics>
    class MakeMCDiscreteArithmeticASEngine_2 {
      public:
        explicit MakeMCDiscreteArithmeticASEngine_2(
            ext::shared_ptr<GeneralizedBlackScholesProcess> process);
        // named parameters
        MakeMCDiscreteArithmeticASEngine_2& withBrownianBridge(bool b = true);
        MakeMCDiscreteArithmeticASEngine_2& withSamples(Size samples);
        MakeMCDiscreteArithmeticASEngine_2& withAbsoluteTolerance(Real tolerance);
        MakeMCDiscreteArithmeticASEngine_2& withMaxSamples(Size samples);
        MakeMCDiscreteArithmeticASEngine_2& withSeed(BigNatural seed);
        MakeMCDiscreteArithmeticASEngine_2& withAntitheticVariate(bool b = true);
        MakeMCDiscreteArithmeticASEngine_2& withConstantParameters(bool constantParameters);
        // conversion to pricing engine
        operator ext::shared_ptr<PricingEngine>() const;
      private:
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        bool antithetic_ = false;
        Size samples_, maxSamples_;
        Real tolerance_;
        bool brownianBridge_ = true;
        BigNatural seed_ = 0;
        bool constantParameters_;
    };

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG, S>::MakeMCDiscreteArithmeticASEngine_2(
        ext::shared_ptr<GeneralizedBlackScholesProcess> process)
    : process_(std::move(process)), samples_(Null<Size>()), maxSamples_(Null<Size>()),
      tolerance_(Null<Real>()) {}

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withSamples(Size samples) {
        QL_REQUIRE(tolerance_ == Null<Real>(),
                   "tolerance already set");
        samples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withAbsoluteTolerance(
                                                             Real tolerance) {
        QL_REQUIRE(samples_ == Null<Size>(),
                   "number of samples already set");
        QL_REQUIRE(RNG::allowsErrorEstimate,
                   "chosen random generator policy "
                   "does not allow an error estimate");
        tolerance_ = tolerance;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withMaxSamples(Size samples) {
        maxSamples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withSeed(BigNatural seed) {
        seed_ = seed;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withBrownianBridge(bool b) {
        brownianBridge_ = b;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withAntitheticVariate(bool b) {
        antithetic_ = b;
        return *this;
    }

      template <class RNG, class S>
    inline MakeMCDiscreteArithmeticASEngine_2<RNG,S>&
        MakeMCDiscreteArithmeticASEngine_2<RNG,S>::withConstantParameters(bool constantParameters) {
        this->constantParameters_ = constantParameters;
        return *this;
    }


    template <class RNG, class S>
    inline
    MakeMCDiscreteArithmeticASEngine_2<RNG,S>::
    operator ext::shared_ptr<PricingEngine>() const {
        return ext::shared_ptr<PricingEngine>(
            new MCDiscreteArithmeticASEngine_2<RNG,S>(process_,
                                                      brownianBridge_,
                                                      antithetic_,
                                                      samples_, tolerance_,
                                                      maxSamples_,
                                                      seed_,
                                                      constantParameters_));
    }

}


#endif
