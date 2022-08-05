declare const enum Result {
    INSTANT = 0,
    SUCCESS = -1,
    FAILURE = -2
}

declare const enum BranchType {
    SEQUENCE,
    SELECTOR
}

declare interface monostate {}
type LeafCallback<C,M> = (ctx: C, memory: M) => Result | number
type DecoratorCallback<C,M> = (ctx: C, memory: M) => Result | number
type Builder<T> = (arg: T) => void

declare interface RootNode<C,LC,DC> {}

declare interface DecorableNode<C,LC,DC,T> extends RootNode<C,LC,DC> {
    Decorate(callback: DecoratorCallback<C,DC>): T
    Decorate(callbacks: DecoratorCallback<C,DC>[]): T
}

declare interface Leaf<C,LC,DC> extends DecorableNode<C,LC,DC,Leaf<C,LC,DC>> {}

declare interface BranchNode<C,LC,DC,T> extends DecorableNode<C,LC,DC,T> {
    AddSequence(builder: Builder<Branch<C,LC,DC>>): T
    AddSelector(builder: Builder<Branch<C,LC,DC>>): T

    AddLeaf(callback: LeafCallback<C,LC>, builder?: Builder<Leaf<C,LC,DC>>);
    AddNode(node: RootNode<C,LC,DC>);

    AddMultiplexer(callback: LeafCallback<C,LC>, builder: Builder<Multiplexer<C,LC,DC>>);
    AddMultiplexer(builder: Builder<Multiplexer<C,LC,DC>>);
}

declare interface Multiplexer<C,LC,DC> extends BranchNode<C,LC,DC,Multiplexer<C,LC,DC>> {}
declare interface Branch<C,LC,DC> extends BranchNode<C,LC,DC,Branch<C,LC,DC>> {
    SetLoops(loops: number): Branch<C,LC,DC>
    SetAttempts(attempts: number): Branch<C,LC,DC>
}
